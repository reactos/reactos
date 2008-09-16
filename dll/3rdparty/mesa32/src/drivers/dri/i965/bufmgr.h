/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef BUFMGR_H
#define BUFMGR_H

#include "intel_context.h"


/* The buffer manager context.  Opaque.
 */
struct bufmgr;
struct buffer;


struct bufmgr *bm_fake_intel_Attach( struct intel_context *intel ); 

/* Flags for validate and other calls.  If both NO_UPLOAD and NO_EVICT
 * are specified, ValidateBuffers is essentially a query.
 */
#define BM_MEM_LOCAL   0x1
#define BM_MEM_AGP     0x2
#define BM_MEM_VRAM    0x4	/* not yet used */
#define BM_WRITE       0x8	/* not yet used */
#define BM_READ        0x10	/* not yet used */
#define BM_NO_UPLOAD   0x20
#define BM_NO_EVICT    0x40
#define BM_NO_MOVE     0x80	/* not yet used */
#define BM_NO_ALLOC    0x100	/* legacy "fixed" buffers only */
#define BM_CLIENT      0x200	/* for map - pointer will be accessed
				 * without dri lock */

#define BM_MEM_MASK (BM_MEM_LOCAL|BM_MEM_AGP|BM_MEM_VRAM)




/* Create a pool of a given memory type, from a certain offset and a
 * certain size.  
 *
 * Also passed in is a virtual pointer to the start of the pool.  This
 * is useful in the faked-out version in i915 so that MapBuffer can
 * return a pointer to a buffer residing in AGP space.  
 *
 * Flags passed into a pool are inherited by all buffers allocated in
 * that pool.  So pools representing the static front,back,depth
 * buffer allocations should have MEM_AGP|NO_UPLOAD|NO_EVICT|NO_MOVE to match
 * the behaviour of the legacy allocations.
 *
 * Returns -1 for failure, pool number for success.
 */
int bmInitPool( struct intel_context *, 
		unsigned long low_offset,
		void *low_virtual,
		unsigned long size,
		unsigned flags);


/* Stick closely to ARB_vbo semantics - they're well defined and
 * understood, and drivers can just pass the calls through without too
 * much thunking.
 */
void bmGenBuffers(struct intel_context *, const char *, unsigned n, struct buffer **buffers,
		  int align );
void bmDeleteBuffers(struct intel_context *, unsigned n, struct buffer **buffers);


/* Hook to inform faked buffer manager about fixed-position
 * front,depth,back buffers.  These may move to a fully memory-managed
 * scheme, or they may continue to be managed as is.
 */
struct buffer *bmGenBufferStatic(struct intel_context *,
				 unsigned pool);

/* On evict, buffer manager will call invalidate_cb() to note that the
 * buffer needs to be reloaded.
 *
 * Buffer is uploaded by calling bmMapBuffer() and copying data into
 * the returned pointer.
 *
 * This is basically a big hack to get some more performance by
 * turning off backing store for buffers where we either have it
 * already (textures) or don't need it (batch buffers, temporary
 * vbo's).
 */
void bmBufferSetInvalidateCB(struct intel_context *,
			     struct buffer *buf,
			     void (*invalidate_cb)( struct intel_context *, void *ptr ),
			     void *ptr,
			     GLboolean dont_fence_subdata);


/* The driver has more intimate knowledge of the hardare than a GL
 * client would, so flags here is more proscriptive than the usage
 * values in the ARB_vbo interface:
 */
int bmBufferData(struct intel_context *, 
		  struct buffer *buf, 
		  unsigned size, 
		  const void *data, 
		  unsigned flags );

int bmBufferSubData(struct intel_context *, 
		     struct buffer *buf, 
		     unsigned offset, 
		     unsigned size, 
		     const void *data );


int bmBufferDataAUB(struct intel_context *, 
		     struct buffer *buf, 
		     unsigned size, 
		     const void *data, 
		     unsigned flags,
		     unsigned aubtype,
		     unsigned aubsubtype );

int bmBufferSubDataAUB(struct intel_context *, 
			struct buffer *buf, 
			unsigned offset, 
			unsigned size, 
			const void *data,
			unsigned aubtype,
			unsigned aubsubtype );


/* In this version, taking the offset will provoke an upload on
 * buffers not already resident in AGP:
 */
unsigned bmBufferOffset(struct intel_context *, 
			struct buffer *buf);


/* Extract data from the buffer:
 */
void bmBufferGetSubData(struct intel_context *, 
			struct buffer *buf, 
			unsigned offset, 
			unsigned size, 
			void *data );

void *bmMapBuffer( struct intel_context *,
		   struct buffer *buf, 
		   unsigned access );

void bmUnmapBuffer( struct intel_context *,
		    struct buffer *buf );

void bmUnmapBufferAUB( struct intel_context *,
		       struct buffer *buf,
		       unsigned aubtype,
		       unsigned aubsubtype );


/* Pertains to all buffers who's offset has been taken since the last
 * fence or release.
 */
int bmValidateBuffers( struct intel_context * );
void bmReleaseBuffers( struct intel_context * );

GLuint bmCtxId( struct intel_context *intel );


GLboolean bmError( struct intel_context * );
void bmEvictAll( struct intel_context * );

void *bmFindVirtual( struct intel_context *intel,
		     unsigned int offset,
		     size_t sz );

/* This functionality is used by the buffer manager, not really sure
 * if we need to be exposing it in this way, probably libdrm will
 * offer equivalent calls.
 *
 * For now they can stay, but will likely change/move before final:
 */
unsigned bmSetFence( struct intel_context * );
unsigned bmSetFenceLock( struct intel_context * );
unsigned bmLockAndFence( struct intel_context *intel );
int bmTestFence( struct intel_context *, unsigned fence );
void bmFinishFence( struct intel_context *, unsigned fence );
void bmFinishFenceLock( struct intel_context *, unsigned fence );

void bm_fake_NotifyContendedLockTake( struct intel_context * );

extern int INTEL_DEBUG;
#define DEBUG_BUFMGR 0x10000000

#define DBG(...)  do { if (INTEL_DEBUG & DEBUG_BUFMGR) _mesa_printf(__VA_ARGS__); } while(0)

#endif
