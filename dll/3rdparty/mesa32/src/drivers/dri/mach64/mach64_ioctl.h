/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#ifndef __MACH64_IOCTL_H__
#define __MACH64_IOCTL_H__

#include "mach64_dri.h"
#include "mach64_reg.h"
#include "mach64_lock.h"

#define MACH64_BUFFER_MAX_DWORDS	(MACH64_BUFFER_SIZE / sizeof(CARD32))


extern drmBufPtr mach64GetBufferLocked( mach64ContextPtr mmesa );
extern void mach64FlushVerticesLocked( mach64ContextPtr mmesa );
extern void mach64FlushDMALocked( mach64ContextPtr mmesa );
extern void mach64UploadHwStateLocked( mach64ContextPtr mmesa );

static __inline void *mach64AllocDmaLow( mach64ContextPtr mmesa, int bytes )
{
   CARD32 *head;

   if ( mmesa->vert_used + bytes > mmesa->vert_total ) {
      LOCK_HARDWARE( mmesa );
      mach64FlushVerticesLocked( mmesa );
      UNLOCK_HARDWARE( mmesa );
   }

   head = (CARD32 *)((char *)mmesa->vert_buf + mmesa->vert_used);
   mmesa->vert_used += bytes;

   return head;
}

static __inline void *mach64AllocDmaLocked( mach64ContextPtr mmesa, int bytes )
{
   CARD32 *head;

   if ( mmesa->vert_used + bytes > mmesa->vert_total ) {
      mach64FlushVerticesLocked( mmesa );
   }

   head = (CARD32 *)((char *)mmesa->vert_buf + mmesa->vert_used);
   mmesa->vert_used += bytes;

   return head;
}

extern void mach64FireBlitLocked( mach64ContextPtr mmesa, void *buffer,
				  GLint offset, GLint pitch, GLint format,
				  GLint x, GLint y, GLint width, GLint height );

extern void mach64CopyBuffer( const __DRIdrawablePrivate *dPriv );
#if ENABLE_PERF_BOXES
extern void mach64PerformanceCounters( mach64ContextPtr mmesa );
extern void mach64PerformanceBoxesLocked( mach64ContextPtr mmesa );
#endif
extern void mach64WaitForIdleLocked( mach64ContextPtr mmesa );

extern void mach64InitIoctlFuncs( struct dd_function_table *functions );

/* ================================================================
 * Helper macros:
 */

#define FLUSH_BATCH( mmesa )						\
do {									\
   if ( MACH64_DEBUG & DEBUG_VERBOSE_IOCTL )				\
      fprintf( stderr, "FLUSH_BATCH in %s\n", __FUNCTION__ );		\
   if ( mmesa->vert_used ) {						\
      mach64FlushVertices( mmesa );					\
   }									\
} while (0)

/* According to a comment in ATIMach64Sync (atimach64.c) in the DDX:
 *
 * "For VTB's and later, the first CPU read of the framebuffer will return
 * zeroes [...] This appears to be due to some kind of engine
 * caching of framebuffer data I haven't found any way of disabling, or
 * otherwise circumventing."
 */
#define FINISH_DMA_LOCKED( mmesa )					\
do {									\
   CARD32 _tmp;								\
   if ( MACH64_DEBUG & DEBUG_VERBOSE_IOCTL )				\
      fprintf( stderr, "FINISH_DMA_LOCKED in %s\n", __FUNCTION__ );	\
   if ( mmesa->vert_used ) {						\
      mach64FlushVerticesLocked( mmesa );				\
   }									\
   mach64WaitForIdleLocked( mmesa );					\
   /* pre-read framebuffer to counter caching problem */		\
   _tmp = *(volatile CARD32 *)mmesa->driScreen->pFB;			\
} while (0)

#define FLUSH_DMA_LOCKED( mmesa )					\
do {									\
   if ( MACH64_DEBUG & DEBUG_VERBOSE_IOCTL )				\
      fprintf( stderr, "FLUSH_DMA_LOCKED in %s\n", __FUNCTION__ );	\
   if ( mmesa->vert_used ) {						\
      mach64FlushVerticesLocked( mmesa );				\
   }									\
   mach64FlushDMALocked( mmesa );					\
} while (0)

#define mach64FlushVertices( mmesa )					\
do {									\
   LOCK_HARDWARE( mmesa );						\
   mach64FlushVerticesLocked( mmesa );					\
   UNLOCK_HARDWARE( mmesa );						\
} while (0)

#define mach64WaitForIdle( mmesa )		\
do {						\
   LOCK_HARDWARE( mmesa );			\
   mach64WaitForIdleLocked( mmesa );		\
   UNLOCK_HARDWARE( mmesa );			\
} while (0)


#endif /* __MACH64_IOCTL_H__ */
