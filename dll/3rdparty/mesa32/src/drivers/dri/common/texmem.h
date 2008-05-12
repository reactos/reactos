/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * (c) Copyright IBM Corporation 2002
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

/** \file texmem.h
 * Public interface to the DRI texture memory management routines.
 * 
 * \sa texmem.c
 */

#ifndef DRI_TEXMEM_H
#define DRI_TEXMEM_H

#include "mtypes.h"
#include "mm.h"
#include "xf86drm.h"

struct dri_tex_heap;
typedef struct dri_tex_heap driTexHeap;

struct dri_texture_object;
typedef struct dri_texture_object driTextureObject;


/**
 * Base texture object type.  Each driver will extend this type with its own
 * private data members.
 */

struct dri_texture_object {
	struct dri_texture_object * next;
	struct dri_texture_object * prev;

	driTexHeap * heap;		/**< Texture heap currently stored in */
	struct gl_texture_object * tObj;/**< Pointer to Mesa texture object
					 * If NULL, this texture object is a
					 * "placeholder" object representing
					 * texture memory in use by another context.
					 * A placeholder should have a heap and a memBlock.
					 */
	struct mem_block *memBlock;	/**< Memory block containing texture */

        unsigned    reserved;	        /**< Cannot be swapped out by user contexts.  */

	unsigned    bound;		/**< Bitmask indicating which tex units
					 * this texture object is bound to.
					 * Bit 0 = unit 0, Bit 1 = unit 1, etc
					 */

	unsigned    totalSize;		/**< Total size of the texture,
					 * including all mipmap levels 
					 */

	unsigned    dirty_images[6];	/**< Flags for whether or not images
					 * need to be uploaded to local or
					 * AGP texture space.  One flag set
					 * for each cube face for cubic
					 * textures.  Bit zero corresponds to
					 * the base-level, which may or may
					 * not be the level zero mipmap.
					 */

        unsigned    timestamp;	        /**< Timestamp used to
					 * synchronize with 3d engine
					 * in hardware where textures
					 * are uploaded directly to
					 * the framebuffer.  
					 */

        unsigned    firstLevel;         /**< Image in \c tObj->Image[0] that
					 * corresponds to the base-level of
					 * this texture object.
					 */

        unsigned    lastLevel;          /**< Last image in \c tObj->Image[0] 
					 * used by the
					 * current LOD settings of
					 * this texture object.  This
					 * value must be greater than
					 * or equal to \c firstLevel.
					 */
};


typedef void (destroy_texture_object_t)( void * driverContext,
				        driTextureObject * t );

/**
 * Client-private representation of texture memory state.
 *
 * Clients will place one or more of these structs in their driver
 * context struct to manage one or more global texture heaps.
 */

struct dri_tex_heap {

	/** Client-supplied heap identifier 
	 */
	unsigned heapId;	

	/** Pointer to the client's private context 
	 */
	void *driverContext;

	/** Total size of the heap, in bytes
	 */
	unsigned size;

	/** \brief \f$log_2\f$ of size of single heap region
	 *
	 * Each context takes memory from the global texture heap in
	 * \f$2^{logGranularity}\f$ byte blocks.  The value of
	 * \a logGranularity is based on the amount of memory represented
	 * by the heap and the maximum number of regions in the SAREA.  Given
	 * \a b bytes of texture memory an \a n regions in the SAREA,
	 * \a logGranularity will be \f$\lfloor\log_2( b / n )\rfloor\f$.
	 */
	unsigned logGranularity;

	/** \brief Required alignment of allocations in this heap
	 * 
	 * The alignment shift is supplied to \a mmAllocMem when memory is
	 * allocated from this heap.  The value of \a alignmentShift will
	 * typically reflect some require of the hardware.  This value has
	 * \b no \b relation to \a logGranularity.  \a alignmentShift is a
	 * per-context value.
	 *
	 * \sa mmAllocMem
	 */
	unsigned alignmentShift;

	/** Number of elements in global list (the SAREA).
	 */
	unsigned nrRegions;	 

	/** Pointer to SAREA \a driTexRegion array
	 */
	drmTextureRegionPtr global_regions;

	/** Pointer to the texture state age (generation number) in the SAREA
	 */
	unsigned     * global_age;

	/** Local age (generation number) of texture state
	 */
	unsigned local_age;

	/** Memory heap used to manage texture memory represented by
	 * this texture heap.
	 */
	struct mem_block * memory_heap;

	/** List of objects that we currently believe to be in texture
	 * memory.
	 */
	driTextureObject     texture_objects;
    
	/** Pointer to the list of texture objects that are not in
	 * texture memory.
	 */
	driTextureObject   * swapped_objects;

	/** Size of the driver-speicific texture object.
	 */
	unsigned       texture_object_size;


	/**
	 * \brief Function to destroy driver-specific texture object data.
	 * 
	 * This function is supplied by the driver so that the texture manager
	 * can release all resources associated with a texture object.  This
	 * function should only release driver-specific data.  That is,
	 * \a driDestroyTextureObject will release the texture memory
	 * associated with the texture object, it will release the memory
	 * for the texture object itself, and it will unlink the texture
	 * object from the texture object lists.
	 *
	 * \param driverContext Pointer to the driver supplied context
	 * \param t Texture object that is to be destroyed
	 * \sa driDestroyTextureObject
	 */

	destroy_texture_object_t * destroy_texture_object;


	/**
	 */
	unsigned * texture_swaps;

        /**
	 * Timestamp used to synchronize with 3d engine in hardware
	 * where textures are uploaded directly to the
	 * framebuffer.  
	 */
        unsigned timestamp;

	/** \brief Kick/upload weight
	 *
	 * When not enough free space is available this weight
	 * influences the choice of the heap from which textures are
	 * kicked. By default the weight is equal to the heap size.
	 */
	double weight;

	/** \brief Kick/upload duty
	 *
	 * The heap with the highest duty will be chosen for kicking
	 * textures if not enough free space is available. The duty is
	 * reduced by the amount of data kicked. Rebalancing of
	 * negative duties takes the weights into account.
	 */
	int duty;
};




/**
 * Called by the client on lock contention to determine whether textures have
 * been stolen.  If another client has modified a region in which we have
 * textures, then we need to figure out which of our textures have been
 * removed and update our global LRU.
 * 
 * \param heap Texture heap to be updated
 * \hideinitializer
 */

#define DRI_AGE_TEXTURES( heap )				\
   do {								\
       if ( ((heap) != NULL)					\
	    && ((heap)->local_age != (heap)->global_age[0]) )	\
	   driAgeTextures( heap );				\
   } while( 0 )




/* This should be called whenever there has been contention on the hardware
 * lock.  driAgeTextures should not be called directly.  Instead, clients
 * should use DRI_AGE_TEXTURES, above.
 */

void driAgeTextures( driTexHeap * heap );

void driUpdateTextureLRU( driTextureObject * t );
void driSwapOutTextureObject( driTextureObject * t );
void driDestroyTextureObject( driTextureObject * t );
int driAllocateTexture( driTexHeap * const * heap_array, unsigned nr_heaps,
    driTextureObject * t );

GLboolean driIsTextureResident( GLcontext * ctx, 
    struct gl_texture_object * texObj );

driTexHeap * driCreateTextureHeap( unsigned heap_id, void * context,
    unsigned size, unsigned alignmentShift, unsigned nr_regions,
    drmTextureRegionPtr global_regions, unsigned * global_age,
    driTextureObject * swapped_objects, unsigned texture_object_size,
    destroy_texture_object_t * destroy_tex_obj );
void driDestroyTextureHeap( driTexHeap * heap );

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
			      int allow_larger_textures );

void
driSetTextureSwapCounterLocation( driTexHeap * heap, unsigned * counter );

#define DRI_TEXMGR_DO_TEXTURE_1D    0x0001
#define DRI_TEXMGR_DO_TEXTURE_2D    0x0002
#define DRI_TEXMGR_DO_TEXTURE_3D    0x0004
#define DRI_TEXMGR_DO_TEXTURE_CUBE  0x0008
#define DRI_TEXMGR_DO_TEXTURE_RECT  0x0010

void driInitTextureObjects( GLcontext *ctx, driTextureObject * swapped,
			    GLuint targets );

GLboolean driValidateTextureHeaps( driTexHeap * const * texture_heaps,
    unsigned nr_heaps, const driTextureObject * swapped );

extern void driCalculateTextureFirstLastLevel( driTextureObject * t );


extern const struct gl_texture_format *_dri_texformat_rgba8888;
extern const struct gl_texture_format *_dri_texformat_argb8888;
extern const struct gl_texture_format *_dri_texformat_rgb565;
extern const struct gl_texture_format *_dri_texformat_argb4444;
extern const struct gl_texture_format *_dri_texformat_argb1555;
extern const struct gl_texture_format *_dri_texformat_al88;
extern const struct gl_texture_format *_dri_texformat_a8;
extern const struct gl_texture_format *_dri_texformat_ci8;
extern const struct gl_texture_format *_dri_texformat_i8;
extern const struct gl_texture_format *_dri_texformat_l8;

extern void driInitTextureFormats( void );

#endif /* DRI_TEXMEM_H */
