/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_ioctl.h,v 1.6 2002/12/16 16:18:53 dawes Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __R128_IOCTL_H__
#define __R128_IOCTL_H__

#include "r128_dri.h"
#include "r128_reg.h"
#include "r128_lock.h"

#define R128_BUFFER_MAX_DWORDS	(R128_BUFFER_SIZE / sizeof(u_int32_t))


extern drmBufPtr r128GetBufferLocked( r128ContextPtr rmesa );
extern void r128FlushVerticesLocked( r128ContextPtr rmesa );

static __inline void *r128AllocDmaLow( r128ContextPtr rmesa, int count,
				       int vert_size )
{
   u_int32_t *head;
   int bytes = count * vert_size;

   if ( !rmesa->vert_buf ) {
      LOCK_HARDWARE( rmesa );
      rmesa->vert_buf = r128GetBufferLocked( rmesa );
      UNLOCK_HARDWARE( rmesa );
   } else if ( rmesa->vert_buf->used + bytes > rmesa->vert_buf->total ) {
      LOCK_HARDWARE( rmesa );
      r128FlushVerticesLocked( rmesa );
      rmesa->vert_buf = r128GetBufferLocked( rmesa );
      UNLOCK_HARDWARE( rmesa );
   }

   head = (u_int32_t *)((char *)rmesa->vert_buf->address + rmesa->vert_buf->used);
   rmesa->vert_buf->used += bytes;
   rmesa->num_verts += count;
   
   return head;
}

extern void r128FireBlitLocked( r128ContextPtr rmesa, drmBufPtr buffer,
				GLint offset, GLint pitch, GLint format,
				GLint x, GLint y, GLint width, GLint height );

extern void r128WriteDepthSpanLocked( r128ContextPtr rmesa,
				      GLuint n, GLint x, GLint y,
				      const GLuint depth[],
				      const GLubyte mask[] );
extern void r128WriteDepthPixelsLocked( r128ContextPtr rmesa, GLuint n,
					const GLint x[], const GLint y[],
					const GLuint depth[],
					const GLubyte mask[] );
extern void r128ReadDepthSpanLocked( r128ContextPtr rmesa,
				     GLuint n, GLint x, GLint y );
extern void r128ReadDepthPixelsLocked( r128ContextPtr rmesa, GLuint n,
				       const GLint x[], const GLint y[] );

extern void r128CopyBuffer( const __DRIdrawablePrivate *dPriv );
extern void r128PageFlip( const __DRIdrawablePrivate *dPriv );
void r128WaitForVBlank( r128ContextPtr rmesa );

extern void r128WaitForIdleLocked( r128ContextPtr rmesa );


extern void r128InitIoctlFuncs( struct dd_function_table *functions );


/* ================================================================
 * Helper macros:
 */

#define FLUSH_BATCH( rmesa )						\
do {									\
   if ( R128_DEBUG & DEBUG_VERBOSE_IOCTL )				\
      fprintf( stderr, "FLUSH_BATCH in %s\n", __FUNCTION__ );		\
   if ( rmesa->vert_buf ) {						\
      r128FlushVertices( rmesa );					\
   }									\
} while (0)

/* 64-bit align the next element address, and then make room for the
 * next indexed prim packet header.
 */
#define ALIGN_NEXT_ELT( rmesa )						\
do {									\
   rmesa->next_elt = (GLushort *)					\
      (((GLuint)rmesa->next_elt + 7) & ~0x7);				\
   rmesa->next_elt = (GLushort *)					\
      ((GLubyte *)rmesa->next_elt + R128_INDEX_PRIM_OFFSET);		\
} while (0)

#define r128FlushVertices( rmesa )					\
do {									\
   LOCK_HARDWARE( rmesa );						\
   r128FlushVerticesLocked( rmesa );					\
   UNLOCK_HARDWARE( rmesa );						\
} while (0)

#define r128FlushElts( rmesa )						\
do {									\
   LOCK_HARDWARE( rmesa );						\
   r128FlushEltsLocked( rmesa );					\
   UNLOCK_HARDWARE( rmesa );						\
} while (0)

#define r128WaitForIdle( rmesa )					\
   do {									\
      LOCK_HARDWARE( rmesa );						\
      r128WaitForIdleLocked( rmesa );					\
      UNLOCK_HARDWARE( rmesa );						\
   } while (0)

#endif /* __R128_IOCTL_H__ */
