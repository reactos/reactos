/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * (C) Copyright IBM Corporation 2002, 2003
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Ian Romanick <idr@us.ibm.com>
 *    Keith Whitwell <keithw@tungstengraphics.com>
 *    Kevin E. Martin <kem@users.sourceforge.net>
 *    Gareth Hughes <gareth@nvidia.com>
 */
/* $XFree86:$ */

/** \file texmem.c
 * Implements all of the device-independent texture memory management.
 * 
 * Currently, only a simple LRU texture memory management policy is
 * implemented.  In the (hopefully very near) future, better policies will be
 * implemented.  The idea is that the DRI should be able to run in one of two
 * modes.  In the default mode the DRI will dynamically attempt to discover
 * the best texture management policy for the running application.  In the
 * other mode, the user (via some sort of as yet TBD mechanism) will select
 * a texture management policy that is known to work well with the
 * application.
 */

#include "texmem.h"
#include "simple_list.h"
#include "imports.h"
#include "macros.h"
#include "texformat.h"

#include <assert.h>



static unsigned dummy_swap_counter;


/**
 * Calculate \f$\log_2\f$ of a value.  This is a particularly poor
 * implementation of this function.  However, since system performance is in
 * no way dependent on this function, the slowness of the implementation is
 * irrelevent.
 * 
 * \param n Value whose \f$\log_2\f$ is to be calculated
 */

static GLuint
driLog2( GLuint n )
{
   GLuint log2;

   for ( log2 = 1 ; n > 1 ; log2++ ) {
      n >>= 1;
   }

   return log2;
}




/**
 * Determine if a texture is resident in textureable memory.  Depending on
 * the driver, this may or may not be on-card memory.  It could be AGP memory
 * or anyother type of memory from which the hardware can directly read
 * texels.
 * 
 * This function is intended to be used as the \c IsTextureResident function
 * in the device's \c dd_function_table.
 * 
 * \param ctx GL context pointer (currently unused)
 * \param texObj Texture object to be tested
 */

GLboolean
driIsTextureResident( GLcontext * ctx, 
		      struct gl_texture_object * texObj )
{
   driTextureObject * t;


   t = (driTextureObject *) texObj->DriverData;
   return( (t != NULL) && (t->memBlock != NULL) );
}




/**
 * (Re)initialize the global circular LRU list.  The last element
 * in the array (\a heap->nrRegions) is the sentinal.  Keeping it
 * at the end of the array allows the other elements of the array
 * to be addressed rationally when looking up objects at a particular
 * location in texture memory.
 * 
 * \param heap Texture heap to be reset
 */

static void resetGlobalLRU( driTexHeap * heap )
{
   drmTextureRegionPtr list = heap->global_regions;
   unsigned       sz = 1U << heap->logGranularity;
   unsigned       i;

   for (i = 0 ; (i+1) * sz <= heap->size ; i++) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = 0;
   }

   i--;
   list[0].prev = heap->nrRegions;
   list[i].prev = i-1;
   list[i].next = heap->nrRegions;
   list[heap->nrRegions].prev = i;
   list[heap->nrRegions].next = 0;
   heap->global_age[0] = 0;
}

/**
 * Print out debugging information about the local texture LRU.
 *
 * \param heap Texture heap to be printed
 * \param callername Name of calling function
 */
static void printLocalLRU( driTexHeap * heap, const char *callername  )
{
   driTextureObject *t;
   unsigned sz = 1U << heap->logGranularity;

   fprintf( stderr, "%s in %s:\nLocal LRU, heap %d:\n", 
	    __FUNCTION__, callername, heap->heapId );

   foreach ( t, &heap->texture_objects ) {
      if (!t->memBlock)
	 continue;
      if (!t->tObj) {
	 fprintf( stderr, "Placeholder (%p) %d at 0x%x sz 0x%x\n",
		  (void *)t,
		  t->memBlock->ofs / sz,
		  t->memBlock->ofs,
		  t->memBlock->size );
      } else {
	 fprintf( stderr, "Texture (%p) at 0x%x sz 0x%x\n",
		  (void *)t,
		  t->memBlock->ofs,
		  t->memBlock->size );
      }
   }
   foreach ( t, heap->swapped_objects ) {
      if (!t->tObj) {
	 fprintf( stderr, "Swapped Placeholder (%p)\n", (void *)t );
      } else {
	 fprintf( stderr, "Swapped Texture (%p)\n", (void *)t );
      }
   }

   fprintf( stderr, "\n" );
}

/**
 * Print out debugging information about the global texture LRU.
 *
 * \param heap Texture heap to be printed
 * \param callername Name of calling function
 */
static void printGlobalLRU( driTexHeap * heap, const char *callername )
{
   drmTextureRegionPtr list = heap->global_regions;
   unsigned int i, j;

   fprintf( stderr, "%s in %s:\nGlobal LRU, heap %d list %p:\n", 
	    __FUNCTION__, callername, heap->heapId, (void *)list );

   for ( i = 0, j = heap->nrRegions ; i < heap->nrRegions ; i++ ) {
      fprintf( stderr, "list[%d] age %d next %d prev %d in_use %d\n",
	       j, list[j].age, list[j].next, list[j].prev, list[j].in_use );
      j = list[j].next;
      if ( j == heap->nrRegions ) break;
   }

   if ( j != heap->nrRegions ) {
      fprintf( stderr, "Loop detected in global LRU\n" );
      for ( i = 0 ; i < heap->nrRegions ; i++ ) {
	 fprintf( stderr, "list[%d] age %d next %d prev %d in_use %d\n",
		  i, list[i].age, list[i].next, list[i].prev, list[i].in_use );
      }
   }

   fprintf( stderr, "\n" );
}


/**
 * Called by the client whenever it touches a local texture.
 * 
 * \param t Texture object that the client has accessed
 */

void driUpdateTextureLRU( driTextureObject * t )
{
   driTexHeap   * heap;
   drmTextureRegionPtr list;
   unsigned   shift;
   unsigned   start;
   unsigned   end;
   unsigned   i;


   heap = t->heap;
   if ( heap != NULL ) {
      shift = heap->logGranularity;
      start = t->memBlock->ofs >> shift;
      end = (t->memBlock->ofs + t->memBlock->size - 1) >> shift;


      heap->local_age = ++heap->global_age[0];
      list = heap->global_regions;


      /* Update the context's local LRU 
       */

      move_to_head( & heap->texture_objects, t );


      for (i = start ; i <= end ; i++) {
	 list[i].age = heap->local_age;

	 /* remove_from_list(i)
	  */
	 list[(unsigned)list[i].next].prev = list[i].prev;
	 list[(unsigned)list[i].prev].next = list[i].next;

	 /* insert_at_head(list, i)
	  */
	 list[i].prev = heap->nrRegions;
	 list[i].next = list[heap->nrRegions].next;
	 list[(unsigned)list[heap->nrRegions].next].prev = i;
	 list[heap->nrRegions].next = i;
      }

      if ( 0 ) {
	 printGlobalLRU( heap, __FUNCTION__ );
	 printLocalLRU( heap, __FUNCTION__ );
      }
   }
}




/**
 * Keep track of swapped out texture objects.
 * 
 * \param t Texture object to be "swapped" out of its texture heap
 */

void driSwapOutTextureObject( driTextureObject * t )
{
   unsigned   face;


   if ( t->memBlock != NULL ) {
      assert( t->heap != NULL );
      mmFreeMem( t->memBlock );
      t->memBlock = NULL;

      if (t->timestamp > t->heap->timestamp)
	 t->heap->timestamp = t->timestamp;

      t->heap->texture_swaps[0]++;
      move_to_tail( t->heap->swapped_objects, t );
      t->heap = NULL;
   }
   else {
      assert( t->heap == NULL );
   }


   for ( face = 0 ; face < 6 ; face++ ) {
      t->dirty_images[face] = ~0;
   }
}




/**
 * Destroy hardware state associated with texture \a t.  Calls the
 * \a destroy_texture_object method associated with the heap from which
 * \a t was allocated.
 * 
 * \param t Texture object to be destroyed
 */

void driDestroyTextureObject( driTextureObject * t )
{
   driTexHeap * heap;


   if ( 0 ) {
      fprintf( stderr, "[%s:%d] freeing %p (tObj = %p, DriverData = %p)\n",
	       __FILE__, __LINE__,
	       (void *)t,
	       (void *)((t != NULL) ? t->tObj : NULL),
	       (void *)((t != NULL && t->tObj != NULL) ? t->tObj->DriverData : NULL ));
   }

   if ( t != NULL ) {
      if ( t->memBlock ) {
	 heap = t->heap;
	 assert( heap != NULL );

	 heap->texture_swaps[0]++;

	 mmFreeMem( t->memBlock );
	 t->memBlock = NULL;

	 if (t->timestamp > t->heap->timestamp)
	    t->heap->timestamp = t->timestamp;

	 heap->destroy_texture_object( heap->driverContext, t );
	 t->heap = NULL;
      }

      if ( t->tObj != NULL ) {
	 assert( t->tObj->DriverData == t );
	 t->tObj->DriverData = NULL;
      }

      remove_from_list( t );
      FREE( t );
   }

   if ( 0 ) {
      fprintf( stderr, "[%s:%d] done freeing %p\n", __FILE__, __LINE__, (void *)t );
   }
}




/**
 * Update the local heap's representation of texture memory based on
 * data in the SAREA.  This is done each time it is detected that some other
 * direct rendering client has held the lock.  This pertains to both our local
 * textures and the textures belonging to other clients.  Keep track of other
 * client's textures by pushing a placeholder texture onto the LRU list --
 * these are denoted by \a tObj being \a NULL.
 * 
 * \param heap Heap whose state is to be updated
 * \param offset Byte offset in the heap that has been stolen
 * \param size Size, in bytes, of the stolen block
 * \param in_use Non-zero if the block is pinned/reserved by the kernel
 */

static void driTexturesGone( driTexHeap * heap, int offset, int size, 
			     int in_use )
{
   driTextureObject * t;
   driTextureObject * tmp;


   foreach_s ( t, tmp, & heap->texture_objects ) {
      if ( (t->memBlock->ofs < (offset + size))
	   && ((t->memBlock->ofs + t->memBlock->size) > offset) ) {
	 /* It overlaps - kick it out.  If the texture object is just a
	  * place holder, then destroy it all together.  Otherwise, mark
	  * it as being swapped out.
	  */

	 if ( t->tObj != NULL ) {
	    driSwapOutTextureObject( t );
	 }
	 else {
	    driDestroyTextureObject( t );
	 }
      }
   }


   {
      t = (driTextureObject *) CALLOC( heap->texture_object_size );
      if ( t == NULL ) return;

      t->memBlock = mmAllocMem( heap->memory_heap, size, 0, offset );
      if ( t->memBlock == NULL ) {
	 fprintf( stderr, "Couldn't alloc placeholder: heap %u sz %x ofs %x\n", heap->heapId,
		  (int)size, (int)offset );
	 mmDumpMemInfo( heap->memory_heap );
	 FREE(t);
	 return;
      }
      t->heap = heap;
      if (in_use) 
	 t->reserved = 1; 
      insert_at_head( & heap->texture_objects, t );
   }
}




/**
 * Called by the client on lock contention to determine whether textures have
 * been stolen.  If another client has modified a region in which we have
 * textures, then we need to figure out which of our textures have been
 * removed and update our global LRU.
 * 
 * \param heap Texture heap to be updated
 */

void driAgeTextures( driTexHeap * heap )
{
   drmTextureRegionPtr list = heap->global_regions;
   unsigned       sz = 1U << (heap->logGranularity);
   unsigned       i, nr = 0;


   /* Have to go right round from the back to ensure stuff ends up
    * LRU in the local list...  Fix with a cursor pointer.
    */

   for (i = list[heap->nrRegions].prev ; 
	i != heap->nrRegions && nr < heap->nrRegions ; 
	i = list[i].prev, nr++) {
      /* If switching texturing schemes, then the SAREA might not have been
       * properly cleared, so we need to reset the global texture LRU.
       */

      if ( (i * sz) > heap->size ) {
	 nr = heap->nrRegions;
	 break;
      }

      if (list[i].age > heap->local_age) 
	  driTexturesGone( heap, i * sz, sz, list[i].in_use); 
   }

   /* Loop or uninitialized heap detected.  Reset.
    */

   if (nr == heap->nrRegions) {
      driTexturesGone( heap, 0, heap->size, 0);
      resetGlobalLRU( heap );
   }

   if ( 0 ) {
      printGlobalLRU( heap, __FUNCTION__ );
      printLocalLRU( heap, __FUNCTION__ );
   }

   heap->local_age = heap->global_age[0];
}




#define INDEX_ARRAY_SIZE 6 /* I'm not aware of driver with more than 2 heaps */

/**
 * Allocate memory from a texture heap to hold a texture object.  This
 * routine will attempt to allocate memory for the texture from the heaps
 * specified by \c heap_array in order.  That is, first it will try to
 * allocate from \c heap_array[0], then \c heap_array[1], and so on.
 *
 * \param heap_array Array of pointers to texture heaps to use
 * \param nr_heaps Number of heap pointer in \a heap_array
 * \param t Texture object for which space is needed
 * \return The ID of the heap from which memory was allocated, or -1 if
 *         memory could not be allocated.
 *
 * \bug The replacement policy implemented by this function is horrible.
 */


int
driAllocateTexture( driTexHeap * const * heap_array, unsigned nr_heaps,
		    driTextureObject * t )
{
   driTexHeap       * heap;
   driTextureObject * temp;
   driTextureObject * cursor;
   unsigned           id;


   /* In case it already has texture space, initialize heap.  This also
    * prevents GCC from issuing a warning that heap might be used
    * uninitialized.
    */

   heap = t->heap;


   /* Run through each of the existing heaps and try to allocate a buffer
    * to hold the texture.
    */

   for ( id = 0 ; (t->memBlock == NULL) && (id < nr_heaps) ; id++ ) {
      heap = heap_array[ id ];
      if ( heap != NULL ) {
	 t->memBlock = mmAllocMem( heap->memory_heap, t->totalSize, 
				   heap->alignmentShift, 0 );
      }
   }


   /* Kick textures out until the requested texture fits.
    */

   if ( t->memBlock == NULL ) {
      unsigned index[INDEX_ARRAY_SIZE];
      unsigned nrGoodHeaps = 0;

      /* Trying to avoid dynamic memory allocation. If you have more
       * heaps, increase INDEX_ARRAY_SIZE. I'm not aware of any
       * drivers with more than 2 tex heaps. */
      assert( nr_heaps < INDEX_ARRAY_SIZE );

      /* Sort large enough heaps by duty. Insertion sort should be
       * fast enough for such a short array. */
      for ( id = 0 ; id < nr_heaps ; id++ ) {
	 heap = heap_array[ id ];

	 if ( heap != NULL && t->totalSize <= heap->size ) {
	    unsigned j;

	    for ( j = 0 ; j < nrGoodHeaps; j++ ) {
	       if ( heap->duty > heap_array[ index[ j ] ]->duty )
		  break;
	    }

	    if ( j < nrGoodHeaps ) {
	       memmove( &index[ j+1 ], &index[ j ],
			sizeof(index[ 0 ]) * (nrGoodHeaps - j) );
	    }

	    index[ j ] = id;

	    nrGoodHeaps++;
	 }
      }

      for ( id = 0 ; (t->memBlock == NULL) && (id < nrGoodHeaps) ; id++ ) {
	 heap = heap_array[ index[ id ] ];

	 for ( cursor = heap->texture_objects.prev, temp = cursor->prev;
	       cursor != &heap->texture_objects ; 
	       cursor = temp, temp = cursor->prev ) {
	       
	    /* The the LRU element.  If the texture is bound to one of
	     * the texture units, then we cannot kick it out.
	     */
	    if ( cursor->bound || cursor->reserved ) {
	       continue;
	    }

	    if ( cursor->memBlock )
	       heap->duty -= cursor->memBlock->size;

	    /* If this is a placeholder, there's no need to keep it */
	    if (cursor->tObj)
	       driSwapOutTextureObject( cursor );
	    else
	       driDestroyTextureObject( cursor );

	    t->memBlock = mmAllocMem( heap->memory_heap, t->totalSize, 
				      heap->alignmentShift, 0 );

	    if (t->memBlock)
	       break;
	 }
      }

      /* Rebalance duties. If a heap kicked more data than its duty,
       * then all other heaps get that amount multiplied with their
       * relative weight added to their duty. The negative duty is
       * reset to 0. In the end all heaps have a duty >= 0.
       *
       * CAUTION: we must not change the heap pointer here, because it
       * is used below to update the texture object.
       */
      for ( id = 0 ; id < nr_heaps ; id++ )
	 if ( heap_array[ id ] != NULL && heap_array[ id ]->duty < 0) {
	    int duty = -heap_array[ id ]->duty;
	    double weight = heap_array[ id ]->weight;
	    unsigned j;

	    for ( j = 0 ; j < nr_heaps ; j++ )
	       if ( j != id && heap_array[ j ] != NULL ) {
		  heap_array[ j ]->duty += (double) duty *
		     heap_array[ j ]->weight / weight;
	       }

	    heap_array[ id ]->duty = 0;
	 }
   }


   if ( t->memBlock != NULL ) {
      /* id and heap->heapId may or may not be the same value here.
       */

      assert( heap != NULL );
      assert( (t->heap == NULL) || (t->heap == heap) );

      t->heap = heap;
      return heap->heapId;
   }
   else {
      assert( t->heap == NULL );

      fprintf( stderr, "[%s:%d] unable to allocate texture\n",
	       __FUNCTION__, __LINE__ );
      return -1;
   }
}






/**
 * Set the location where the texture-swap counter is stored.
 */

void
driSetTextureSwapCounterLocation( driTexHeap * heap, unsigned * counter )
{
   heap->texture_swaps = (counter == NULL) ? & dummy_swap_counter : counter;
}




/**
 * Create a new heap for texture data.
 * 
 * \param heap_id             Device-dependent heap identifier.  This value
 *                            will returned by driAllocateTexture when memory
 *                            is allocated from this heap.
 * \param context             Device-dependent driver context.  This is
 *                            supplied as the first parameter to the
 *                            \c destroy_tex_obj function.
 * \param size                Size, in bytes, of the texture region
 * \param alignmentShift      Alignment requirement for textures.  If textures 
 *                            must be allocated on a 4096 byte boundry, this
 *                            would be 12.
 * \param nr_regions          Number of regions into which this texture space
 *                            should be partitioned
 * \param global_regions      Array of \c drmTextureRegion structures in the SAREA
 * \param global_age          Pointer to the global texture age in the SAREA
 * \param swapped_objects     Pointer to the list of texture objects that are
 *                            not in texture memory (i.e., have been swapped
 *                            out).
 * \param texture_object_size Size, in bytes, of a device-dependent texture
 *                            object
 * \param destroy_tex_obj     Function used to destroy a device-dependent
 *                            texture object
 *
 * \sa driDestroyTextureHeap
 */

driTexHeap *
driCreateTextureHeap( unsigned heap_id, void * context, unsigned size,
		      unsigned alignmentShift, unsigned nr_regions,
		      drmTextureRegionPtr global_regions, unsigned * global_age,
		      driTextureObject * swapped_objects, 
		      unsigned texture_object_size,
		      destroy_texture_object_t * destroy_tex_obj
		    )
{
   driTexHeap * heap;
   unsigned     l;
    
    
   if ( 0 )
       fprintf( stderr, "%s( %u, %p, %u, %u, %u )\n",
		__FUNCTION__,
		heap_id, (void *)context, size, alignmentShift, nr_regions );

   heap = (driTexHeap *) CALLOC( sizeof( driTexHeap ) );
   if ( heap != NULL ) {
      l = driLog2( (size - 1) / nr_regions );
      if ( l < alignmentShift )
      {
	 l = alignmentShift;
      }

      heap->logGranularity = l;
      heap->size = size & ~((1L << l) - 1);

      heap->memory_heap = mmInit( 0, heap->size );
      if ( heap->memory_heap != NULL ) {
	 heap->heapId = heap_id;
	 heap->driverContext = context;

	 heap->alignmentShift = alignmentShift;
	 heap->nrRegions = nr_regions;
	 heap->global_regions = global_regions;
	 heap->global_age = global_age;
	 heap->swapped_objects = swapped_objects;
	 heap->texture_object_size = texture_object_size;
	 heap->destroy_texture_object = destroy_tex_obj;

	 /* Force global heap init */
	 if (heap->global_age[0] == 0)
	     heap->local_age = ~0;
	 else
	     heap->local_age = 0;

	 make_empty_list( & heap->texture_objects );
	 driSetTextureSwapCounterLocation( heap, NULL );

	 heap->weight = heap->size;
	 heap->duty = 0;
      }
      else {
	 FREE( heap );
	 heap = NULL;
      }
   }


   if ( 0 )
       fprintf( stderr, "%s returning %p\n", __FUNCTION__, (void *)heap );

   return heap;
}




/** Destroys a texture heap
 * 
 * \param heap Texture heap to be destroyed
 */

void
driDestroyTextureHeap( driTexHeap * heap )
{
   driTextureObject * t;
   driTextureObject * temp;


   if ( heap != NULL ) {
      foreach_s( t, temp, & heap->texture_objects ) {
	 driDestroyTextureObject( t );
      }
      foreach_s( t, temp, heap->swapped_objects ) {
	 driDestroyTextureObject( t );
      }

      mmDestroy( heap->memory_heap );
      FREE( heap );
   }
}




/****************************************************************************/
/**
 * Determine how many texels (including all mipmap levels) would be required
 * for a texture map of size \f$2^^\c base_size_log2\f$ would require.
 *
 * \param base_size_log2 \f$log_2\f$ of the size of a side of the texture
 * \param dimensions Number of dimensions of the texture.  Either 2 or 3.
 * \param faces Number of faces of the texture.  Either 1 or 6 (for cube maps).
 * \return Number of texels
 */

static unsigned
texels_this_map_size( int base_size_log2, unsigned dimensions, unsigned faces )
{
   unsigned  texels;


   assert( (faces == 1) || (faces == 6) );
   assert( (dimensions == 2) || (dimensions == 3) );

   texels = 0;
   if ( base_size_log2 >= 0 ) {
      texels = (1U << (dimensions * base_size_log2));

      /* See http://www.mail-archive.com/dri-devel@lists.sourceforge.net/msg03636.html
       * for the complete explaination of why this formulation is used.
       * Basically, the smaller mipmap levels sum to 0.333 the size of the
       * level 0 map.  The total size is therefore the size of the map
       * multipled by 1.333.  The +2 is there to round up.
       */

      texels = (texels * 4 * faces + 2) / 3;
   }

   return texels;
}




struct maps_per_heap {
   unsigned  c[32];
};

static void
fill_in_maximums( driTexHeap * const * heaps, unsigned nr_heaps,
		  unsigned max_bytes_per_texel, unsigned max_size,
		  unsigned mipmaps_at_once, unsigned dimensions,
		  unsigned faces, struct maps_per_heap * max_textures )
{
   unsigned   heap;
   unsigned   log2_size;
   unsigned   mask;


   /* Determine how many textures of each size can be stored in each
    * texture heap.
    */

   for ( heap = 0 ; heap < nr_heaps ; heap++ ) {
      if ( heaps[ heap ] == NULL ) {
	 (void) memset( max_textures[ heap ].c, 0, 
			sizeof( max_textures[ heap ].c ) );
	 continue;
      }

      mask = (1U << heaps[ heap ]->logGranularity) - 1;

      if ( 0 ) {
	 fprintf( stderr, "[%s:%d] heap[%u] = %u bytes, mask = 0x%08x\n",
		  __FILE__, __LINE__,
		  heap, heaps[ heap ]->size, mask );
      }

      for ( log2_size = max_size ; log2_size > 0 ; log2_size-- ) {
	 unsigned   total;


	 /* Determine the total number of bytes required by a texture of
	  * size log2_size.
	  */

	 total = texels_this_map_size( log2_size, dimensions, faces )
	     - texels_this_map_size( log2_size - mipmaps_at_once,
				     dimensions, faces );
	 total *= max_bytes_per_texel;
	 total = (total + mask) & ~mask;

	 /* The number of textures of a given size that will fit in a heap
	  * is equal to the size of the heap divided by the size of the
	  * texture.
	  */

	 max_textures[ heap ].c[ log2_size ] = heaps[ heap ]->size / total;

	 if ( 0 ) {
	    fprintf( stderr, "[%s:%d] max_textures[%u].c[%02u] "
		     "= 0x%08x / 0x%08x "
		     "= %u (%u)\n",
		     __FILE__, __LINE__,
		     heap, log2_size,
		     heaps[ heap ]->size, total,
		     heaps[ heap ]->size / total,
		     max_textures[ heap ].c[ log2_size ] );
	 }
      }
   }
}


static unsigned
get_max_size( unsigned nr_heaps,
	      unsigned texture_units,
	      unsigned max_size,
	      int all_textures_one_heap,
	      struct maps_per_heap * max_textures )
{
   unsigned   heap;
   unsigned   log2_size;


   /* Determine the largest texture size such that a texture of that size
    * can be bound to each texture unit at the same time.  Some hardware
    * may require that all textures be in the same texture heap for
    * multitexturing.
    */

   for ( log2_size = max_size ; log2_size > 0 ; log2_size-- ) {
      unsigned   total = 0;

      for ( heap = 0 ; heap < nr_heaps ; heap++ )
      {
	 total += max_textures[ heap ].c[ log2_size ];

	 if ( 0 ) {
	    fprintf( stderr, "[%s:%d] max_textures[%u].c[%02u] = %u, "
		     "total = %u\n", __FILE__, __LINE__, heap, log2_size,
		     max_textures[ heap ].c[ log2_size ], total );
	 }

	 if ( (max_textures[ heap ].c[ log2_size ] >= texture_units)
	      || (!all_textures_one_heap && (total >= texture_units)) ) {
	    /* The number of mipmap levels is the log-base-2 of the
	     * maximum texture size plus 1.  If the maximum texture size
	     * is 1x1, the log-base-2 is 0 and 1 mipmap level (the base
	     * level) is available.
	     */

	    return log2_size + 1;
	 }
      }
   }

   /* This should NEVER happen.  It should always be possible to have at
    * *least* a 1x1 texture in memory!
    */
   assert( log2_size != 0 );
   return 0;
}

#define SET_MAX(f,v) \
    do { if ( max_sizes[v] != 0 ) { limits-> f = max_sizes[v]; } } while( 0 )

#define SET_MAX_RECT(f,v) \
    do { if ( max_sizes[v] != 0 ) { limits-> f = 1 << (max_sizes[v] - 1); } } while( 0 )


/**
 * Given the amount of texture memory, the number of texture units, and the
 * maximum size of a texel, calculate the maximum texture size the driver can
 * advertise.
 * 
 * \param heaps Texture heaps for this card
 * \param nr_heap Number of texture heaps
 * \param limits OpenGL contants.  MaxTextureUnits must be set.
 * \param max_bytes_per_texel Maximum size of a single texel, in bytes
 * \param max_2D_size \f$\log_2\f$ of the maximum 2D texture size (i.e.,
 *     1024x1024 textures, this would be 10)
 * \param max_3D_size \f$\log_2\f$ of the maximum 3D texture size (i.e.,
 *     1024x1024x1024 textures, this would be 10)
 * \param max_cube_size \f$\log_2\f$ of the maximum cube texture size (i.e.,
 *     1024x1024 textures, this would be 10)
 * \param max_rect_size \f$\log_2\f$ of the maximum texture rectangle size
 *     (i.e., 1024x1024 textures, this would be 10).  This is a power-of-2
 *     even though texture rectangles need not be a power-of-2.
 * \param mipmaps_at_once Total number of mipmaps that can be used
 *     at one time.  For most hardware this will be \f$\c max_size + 1\f$.
 *     For hardware that does not support mipmapping, this will be 1.
 * \param all_textures_one_heap True if the hardware requires that all
 *     textures be in a single texture heap for multitexturing.
 * \param allow_larger_textures 0 conservative, 1 calculate limits
 *     so at least one worst-case texture can fit, 2 just use hw limits.
 */

void
driCalculateMaxTextureLevels( driTexHeap * const * heaps,
			      unsigned nr_heaps,
			      struct gl_constants * limits,
			      unsigned max_bytes_per_texel,
			      unsigned max_2D_size,
			      unsigned max_3D_size,
			      unsigned max_cube_size,
			      unsigned max_rect_size,
			      unsigned mipmaps_at_once,
			      int all_textures_one_heap,
			      int allow_larger_textures )
{
   struct maps_per_heap  max_textures[8];
   unsigned         i;
   const unsigned   dimensions[4] = { 2, 3, 2, 2 };
   const unsigned   faces[4]      = { 1, 1, 6, 1 };
   unsigned         max_sizes[4];
   unsigned         mipmaps[4];


   max_sizes[0] = max_2D_size;
   max_sizes[1] = max_3D_size;
   max_sizes[2] = max_cube_size;
   max_sizes[3] = max_rect_size;

   mipmaps[0] = mipmaps_at_once;
   mipmaps[1] = mipmaps_at_once;
   mipmaps[2] = mipmaps_at_once;
   mipmaps[3] = 1;


   /* Calculate the maximum number of texture levels in two passes.  The
    * first pass determines how many textures of each power-of-two size
    * (including all mipmap levels for that size) can fit in each texture
    * heap.  The second pass finds the largest texture size that allows
    * a texture of that size to be bound to every texture unit.
    */

   for ( i = 0 ; i < 4 ; i++ ) {
      if ( (allow_larger_textures != 2) && (max_sizes[ i ] != 0) ) {
	 fill_in_maximums( heaps, nr_heaps, max_bytes_per_texel,
			   max_sizes[ i ], mipmaps[ i ],
			   dimensions[ i ], faces[ i ],
			   max_textures );

	 max_sizes[ i ] = get_max_size( nr_heaps,
					allow_larger_textures == 1 ?
					1 : limits->MaxTextureUnits,
					max_sizes[ i ],
					all_textures_one_heap,
					max_textures );
      }
      else if (max_sizes[ i ] != 0) {
	 max_sizes[ i ] += 1;
      }
   }

   SET_MAX( MaxTextureLevels,        0 );
   SET_MAX( Max3DTextureLevels,      1 );
   SET_MAX( MaxCubeTextureLevels,    2 );
   SET_MAX_RECT( MaxTextureRectSize, 3 );
}




/**
 * Perform initial binding of default textures objects on a per unit, per
 * texture target basis.
 *
 * \param ctx Current OpenGL context
 * \param swapped List of swapped-out textures
 * \param targets Bit-mask of value texture targets
 */

void driInitTextureObjects( GLcontext *ctx, driTextureObject * swapped,
			    GLuint targets )
{
   struct gl_texture_object *texObj;
   GLuint tmp = ctx->Texture.CurrentUnit;
   unsigned   i;


   for ( i = 0 ; i < ctx->Const.MaxTextureUnits ; i++ ) {
      ctx->Texture.CurrentUnit = i;

      if ( (targets & DRI_TEXMGR_DO_TEXTURE_1D) != 0 ) {
	 texObj = ctx->Texture.Unit[i].Current1D;
	 ctx->Driver.BindTexture( ctx, GL_TEXTURE_1D, texObj );
	 move_to_tail( swapped, (driTextureObject *) texObj->DriverData );
      }

      if ( (targets & DRI_TEXMGR_DO_TEXTURE_2D) != 0 ) {
	 texObj = ctx->Texture.Unit[i].Current2D;
	 ctx->Driver.BindTexture( ctx, GL_TEXTURE_2D, texObj );
	 move_to_tail( swapped, (driTextureObject *) texObj->DriverData );
      }

      if ( (targets & DRI_TEXMGR_DO_TEXTURE_3D) != 0 ) {
	 texObj = ctx->Texture.Unit[i].Current3D;
	 ctx->Driver.BindTexture( ctx, GL_TEXTURE_3D, texObj );
	 move_to_tail( swapped, (driTextureObject *) texObj->DriverData );
      }

      if ( (targets & DRI_TEXMGR_DO_TEXTURE_CUBE) != 0 ) {
	 texObj = ctx->Texture.Unit[i].CurrentCubeMap;
	 ctx->Driver.BindTexture( ctx, GL_TEXTURE_CUBE_MAP_ARB, texObj );
	 move_to_tail( swapped, (driTextureObject *) texObj->DriverData );
      }

      if ( (targets & DRI_TEXMGR_DO_TEXTURE_RECT) != 0 ) {
	 texObj = ctx->Texture.Unit[i].CurrentRect;
	 ctx->Driver.BindTexture( ctx, GL_TEXTURE_RECTANGLE_NV, texObj );
	 move_to_tail( swapped, (driTextureObject *) texObj->DriverData );
      }
   }

   ctx->Texture.CurrentUnit = tmp;
}




/**
 * Verify that the specified texture is in the specificed heap.
 * 
 * \param tex   Texture to be tested.
 * \param heap  Texture memory heap to be tested.
 * \return True if the texture is in the heap, false otherwise.
 */

static GLboolean
check_in_heap( const driTextureObject * tex, const driTexHeap * heap )
{
#if 1
   return tex->heap == heap;
#else
   driTextureObject * curr;

   foreach( curr, & heap->texture_objects ) {
      if ( curr == tex ) {
	 break;
      }
   }

   return curr == tex;
#endif
}



/****************************************************************************/
/**
 * Validate the consistency of a set of texture heaps.
 * Original version by Keith Whitwell in r200/r200_sanity.c.
 */

GLboolean
driValidateTextureHeaps( driTexHeap * const * texture_heaps,
			 unsigned nr_heaps, const driTextureObject * swapped )
{
   driTextureObject *t;
   unsigned  i;

   for ( i = 0 ; i < nr_heaps ; i++ ) {
      int last_end = 0;
      unsigned textures_in_heap = 0;
      unsigned blocks_in_mempool = 0;
      const driTexHeap * heap = texture_heaps[i];
      const struct mem_block *p = heap->memory_heap;

      /* Check each texture object has a MemBlock, and is linked into
       * the correct heap.  
       *
       * Check the texobj base address corresponds to the MemBlock
       * range.  Check the texobj size (recalculate?) fits within
       * the MemBlock.
       *
       * Count the number of texobj's using this heap.
       */

      foreach ( t, &heap->texture_objects ) {
	 if ( !check_in_heap( t, heap ) ) {
	    fprintf( stderr, "%s memory block for texture object @ %p not "
		     "found in heap #%d\n",
		     __FUNCTION__, (void *)t, i );
	    return GL_FALSE;
	 }


	 if ( t->totalSize > t->memBlock->size ) {
	    fprintf( stderr, "%s: Memory block for texture object @ %p is "
		     "only %u bytes, but %u are required\n",
		     __FUNCTION__, (void *)t, t->totalSize, t->memBlock->size );
	    return GL_FALSE;
	 }

	 textures_in_heap++;
      }

      /* Validate the contents of the heap:
       *   - Ordering
       *   - Overlaps
       *   - Bounds
       */

      while ( p != NULL ) {
	 if (p->reserved) {
	    fprintf( stderr, "%s: Block (%08x,%x), is reserved?!\n",
		     __FUNCTION__, p->ofs, p->size );
	    return GL_FALSE;
	 }

	 if (p->ofs != last_end) {
	    fprintf( stderr, "%s: blocks_in_mempool = %d, last_end = %d, p->ofs = %d\n",
		     __FUNCTION__, blocks_in_mempool, last_end, p->ofs );
	    return GL_FALSE;
	 }

	 if (!p->reserved && !p->free) {
	    blocks_in_mempool++;
	 }

	 last_end = p->ofs + p->size;
	 p = p->next;
      }

      if (textures_in_heap != blocks_in_mempool) {
	 fprintf( stderr, "%s: Different number of textures objects (%u) and "
		  "inuse memory blocks (%u)\n", 
		  __FUNCTION__, textures_in_heap, blocks_in_mempool );
	 return GL_FALSE;
      }

#if 0
      fprintf( stderr, "%s: textures_in_heap = %u\n", 
	       __FUNCTION__, textures_in_heap );
#endif
   }


   /* Check swapped texobj's have zero memblocks
    */
   i = 0;
   foreach ( t, swapped ) {
      if ( t->memBlock != NULL ) {
	 fprintf( stderr, "%s: Swapped texobj %p has non-NULL memblock %p\n",
		  __FUNCTION__, (void *)t, (void *)t->memBlock );
	 return GL_FALSE;
      }
      i++;
   }

#if 0
   fprintf( stderr, "%s: swapped texture count = %u\n", __FUNCTION__, i );
#endif

   return GL_TRUE;
}




/****************************************************************************/
/**
 * Compute which mipmap levels that really need to be sent to the hardware.
 * This depends on the base image size, GL_TEXTURE_MIN_LOD,
 * GL_TEXTURE_MAX_LOD, GL_TEXTURE_BASE_LEVEL, and GL_TEXTURE_MAX_LEVEL.
 */

void
driCalculateTextureFirstLastLevel( driTextureObject * t )
{
   struct gl_texture_object * const tObj = t->tObj;
   const struct gl_texture_image * const baseImage =
       tObj->Image[0][tObj->BaseLevel];

   /* These must be signed values.  MinLod and MaxLod can be negative numbers,
    * and having firstLevel and lastLevel as signed prevents the need for
    * extra sign checks.
    */
   int   firstLevel;
   int   lastLevel;

   /* Yes, this looks overly complicated, but it's all needed.
    */

   switch (tObj->Target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
      if (tObj->MinFilter == GL_NEAREST || tObj->MinFilter == GL_LINEAR) {
         /* GL_NEAREST and GL_LINEAR only care about GL_TEXTURE_BASE_LEVEL.
          */

         firstLevel = lastLevel = tObj->BaseLevel;
      }
      else {
	 firstLevel = tObj->BaseLevel + (GLint)(tObj->MinLod + 0.5);
	 firstLevel = MAX2(firstLevel, tObj->BaseLevel);
	 lastLevel = tObj->BaseLevel + (GLint)(tObj->MaxLod + 0.5);
	 lastLevel = MAX2(lastLevel, t->tObj->BaseLevel);
	 lastLevel = MIN2(lastLevel, t->tObj->BaseLevel + baseImage->MaxLog2);
	 lastLevel = MIN2(lastLevel, t->tObj->MaxLevel);
	 lastLevel = MAX2(firstLevel, lastLevel); /* need at least one level */
      }
      break;
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_4D_SGIS:
      firstLevel = lastLevel = 0;
      break;
   default:
      return;
   }

   /* save these values */
   t->firstLevel = firstLevel;
   t->lastLevel = lastLevel;
}




/**
 * \name DRI texture formats.  Pointers initialized to either the big- or
 * little-endian Mesa formats.
 */
/*@{*/
const struct gl_texture_format *_dri_texformat_rgba8888 = NULL;
const struct gl_texture_format *_dri_texformat_argb8888 = NULL;
const struct gl_texture_format *_dri_texformat_rgb565 = NULL;
const struct gl_texture_format *_dri_texformat_argb4444 = NULL;
const struct gl_texture_format *_dri_texformat_argb1555 = NULL;
const struct gl_texture_format *_dri_texformat_al88 = NULL;
const struct gl_texture_format *_dri_texformat_a8 = &_mesa_texformat_a8;
const struct gl_texture_format *_dri_texformat_ci8 = &_mesa_texformat_ci8;
const struct gl_texture_format *_dri_texformat_i8 = &_mesa_texformat_i8;
const struct gl_texture_format *_dri_texformat_l8 = &_mesa_texformat_l8;
/*@}*/


/**
 * Initialize little endian target, host byte order independent texture formats
 */
void
driInitTextureFormats(void)
{
   const GLuint ui = 1;
   const GLubyte littleEndian = *((const GLubyte *) &ui);

   if (littleEndian) {
      _dri_texformat_rgba8888	= &_mesa_texformat_rgba8888;
      _dri_texformat_argb8888	= &_mesa_texformat_argb8888;
      _dri_texformat_rgb565	= &_mesa_texformat_rgb565;
      _dri_texformat_argb4444	= &_mesa_texformat_argb4444;
      _dri_texformat_argb1555	= &_mesa_texformat_argb1555;
      _dri_texformat_al88	= &_mesa_texformat_al88;
   }
   else {
      _dri_texformat_rgba8888	= &_mesa_texformat_rgba8888_rev;
      _dri_texformat_argb8888	= &_mesa_texformat_argb8888_rev;
      _dri_texformat_rgb565	= &_mesa_texformat_rgb565_rev;
      _dri_texformat_argb4444	= &_mesa_texformat_argb4444_rev;
      _dri_texformat_argb1555	= &_mesa_texformat_argb1555_rev;
      _dri_texformat_al88	= &_mesa_texformat_al88_rev;
   }
}
