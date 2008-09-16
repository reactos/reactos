/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _VIAIOCTL_H
#define _VIAIOCTL_H

#include "via_context.h"


void viaFinishPrimitive(struct via_context *vmesa);
void viaFlushDma(struct via_context *vmesa);
void viaFlushDmaLocked(struct via_context *vmesa, GLuint flags);

void viaInitIoctlFuncs(GLcontext *ctx);
void viaCopyBuffer(const __DRIdrawablePrivate *dpriv);
void viaPageFlip(const __DRIdrawablePrivate *dpriv);
void viaCheckDma(struct via_context *vmesa, GLuint bytes);
void viaResetPageFlippingLocked(struct via_context *vmesa);
void viaWaitIdle(struct via_context *vmesa, GLboolean light);
void viaWaitIdleLocked(struct via_context *vmesa, GLboolean light);

GLboolean viaCheckBreadcrumb( struct via_context *vmesa, GLuint value );
void viaEmitBreadcrumb( struct via_context *vmesa );


#define VIA_FINISH_PRIM(vmesa) do {		\
   if (vmesa->dmaLastPrim)			\
      viaFinishPrimitive( vmesa );		\
} while (0)

#define VIA_FLUSH_DMA(vmesa) do {		\
   VIA_FINISH_PRIM(vmesa);			\
   if (vmesa->dmaLow) 		\
      viaFlushDma(vmesa);			\
} while (0)
    

void viaWrapPrimitive( struct via_context *vmesa );

static __inline__ GLuint *viaAllocDma(struct via_context *vmesa, int bytes)
{
   if (vmesa->dmaLow + bytes > VIA_DMA_HIGHWATER) {
      viaFlushDma(vmesa);
   }

   {
      GLuint *start = (GLuint *)(vmesa->dma + vmesa->dmaLow);
      vmesa->dmaLow += bytes;
      return start;
   }
}


static GLuint __inline__ *viaExtendPrimitive(struct via_context *vmesa, int bytes)
{
   if (vmesa->dmaLow + bytes > VIA_DMA_HIGHWATER) {
      viaWrapPrimitive(vmesa);
   }

   {
      GLuint *start = (GLuint *)(vmesa->dma + vmesa->dmaLow);
      vmesa->dmaLow += bytes;
      return start;
   }
}




#define RING_VARS GLuint *_vb = 0, _nr, _x;

#define BEGIN_RING(n) do {				\
   if (_vb != 0) abort();				\
   _vb = viaAllocDma(vmesa, (n) * sizeof(GLuint));	\
   _nr = (n);						\
   _x = 0;						\
} while (0)

#define BEGIN_RING_NOCHECK(n) do {			\
   if (_vb != 0) abort();				\
   _vb = (GLuint *)(vmesa->dma + vmesa->dmaLow);	\
   vmesa->dmaLow += (n) * sizeof(GLuint);		\
   _nr = (n);						\
   _x = 0;						\
} while (0)

#define OUT_RING(n) _vb[_x++] = (n)

#define ADVANCE_RING() do {			\
   if (_x != _nr) abort(); 			\
   _vb = 0;						\
} while (0)

#define ADVANCE_RING_VARIABLE() do {			\
   if (_x > _nr) abort();				\
   vmesa->dmaLow -= (_nr - _x) * sizeof(GLuint);	\
   _vb = 0;						\
} while (0)


#define QWORD_PAD_RING() do {			\
   if (vmesa->dmaLow & 0x4) {			\
      BEGIN_RING(1);				\
      OUT_RING(HC_DUMMY);			\
      ADVANCE_RING();				\
   }						\
} while (0)

#define VIA_GEQ_WRAP(left, right) \
    (((left) - (right)) < ( 1 << 23))
      
#endif
