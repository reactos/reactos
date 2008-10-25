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

#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "intel_context.h"
#include "intel_ioctl.h"


#define BATCH_LOCALS	GLubyte *batch_ptr;

/* #define VERBOSE 0 */
#ifndef VERBOSE
extern int VERBOSE;
#endif


#define BEGIN_BATCH(n)							\
do {									\
   if (VERBOSE) fprintf(stderr, 					\
			"BEGIN_BATCH(%ld) in %s, %d dwords free\n",	\
			((unsigned long)n), __FUNCTION__,		\
			intel->batch.space/4);				\
   if (intel->batch.space < (n)*4)					\
      intelFlushBatch(intel, GL_TRUE);					\
   if (intel->batch.space == intel->batch.size)	intel->batch.func = __FUNCTION__;			\
   batch_ptr = intel->batch.ptr;					\
} while (0)

#define OUT_BATCH(n)					\
do {							\
   *(GLuint *)batch_ptr = (n);				\
   if (VERBOSE) fprintf(stderr, " -- %08x at %s/%d\n", (n), __FILE__, __LINE__);	\
   batch_ptr += 4;					\
} while (0)

#define ADVANCE_BATCH()						\
do {								\
   if (VERBOSE) fprintf(stderr, "ADVANCE_BATCH()\n");		\
   intel->batch.space -= (batch_ptr - intel->batch.ptr);	\
   intel->batch.ptr = batch_ptr;				\
   assert(intel->batch.space >= 0);				\
} while(0)

extern void intelInitBatchBuffer( GLcontext *ctx );
extern void intelDestroyBatchBuffer( GLcontext *ctx );

extern void intelStartInlinePrimitive( intelContextPtr intel, GLuint prim );
extern void intelWrapInlinePrimitive( intelContextPtr intel );
extern void intelRestartInlinePrimitive( intelContextPtr intel );
extern GLuint *intelEmitInlinePrimitiveLocked(intelContextPtr intel, 
					      int primitive, int dwords,
					      int vertex_size);
extern void intelCopyBuffer( const __DRIdrawablePrivate *dpriv,
			     const drm_clip_rect_t	*rect);
extern void intelClearWithBlit(GLcontext *ctx, GLbitfield mask, GLboolean all,
			     GLint cx1, GLint cy1, GLint cw, GLint ch);

extern void intelEmitCopyBlitLocked( intelContextPtr intel,
				     GLuint cpp,
				     GLshort src_pitch,
				     GLuint  src_offset,
				     GLshort dst_pitch,
				     GLuint  dst_offset,
				     GLshort srcx, GLshort srcy,
				     GLshort dstx, GLshort dsty,
				     GLshort w, GLshort h );

extern void intelEmitFillBlitLocked( intelContextPtr intel,
				     GLuint cpp,
				     GLshort dst_pitch,
				     GLuint dst_offset,
				     GLshort x, GLshort y, 
				     GLshort w, GLshort h,
				     GLuint color );




static __inline GLuint *intelExtendInlinePrimitive( intelContextPtr intel, 
						GLuint dwords )
{
   GLuint sz = dwords * sizeof(GLuint);
   GLuint *ptr;

   if (intel->batch.space < sz) {
      intelWrapInlinePrimitive( intel );
/*       assert(intel->batch.space >= sz); */
   }

/*    assert(intel->prim.primitive != ~0); */
   ptr = (GLuint *)intel->batch.ptr;
   intel->batch.ptr += sz;
   intel->batch.space -= sz;

   return ptr;
}



#endif
