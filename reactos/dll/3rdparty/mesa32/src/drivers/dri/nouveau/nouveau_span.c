/**************************************************************************

Copyright 2006 Stephane Marchesin
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/


#include "nouveau_context.h"
#include "nouveau_span.h"
#include "nouveau_fifo.h"
#include "nouveau_lock.h"

#include "swrast/swrast.h"

#define HAVE_HW_DEPTH_SPANS	0
#define HAVE_HW_DEPTH_PIXELS	0
#define HAVE_HW_STENCIL_SPANS	0
#define HAVE_HW_STENCIL_PIXELS	0

#define HW_CLIPLOOP()							\
   do {									\
      int _nc = nmesa->numClipRects;					\
      while ( _nc-- ) {							\
	 int minx = nmesa->pClipRects[_nc].x1 - nmesa->drawX;		\
	 int miny = nmesa->pClipRects[_nc].y1 - nmesa->drawY;		\
	 int maxx = nmesa->pClipRects[_nc].x2 - nmesa->drawX;		\
	 int maxy = nmesa->pClipRects[_nc].y2 - nmesa->drawY;

#define LOCAL_VARS							\
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);			\
   nouveau_renderbuffer *nrb = (nouveau_renderbuffer *)rb;		\
   GLuint height = nrb->mesa.Height;					\
   GLubyte *map = (GLubyte *)(nrb->map ? nrb->map : nrb->mem->map) +    \
	 (nmesa->drawY * nrb->pitch) + (nmesa->drawX * nrb->cpp);       \
   GLuint p;								\
   (void) p;

#define Y_FLIP( _y )            (height - _y - 1)

#define HW_LOCK()

#define HW_UNLOCK()



/* ================================================================
 * Color buffers
 */

/* RGB565 */ 
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    nouveau##x##_RGB565
#define TAG2(x,y) nouveau##x##_RGB565##y
#define GET_PTR(X,Y) (map + (Y)*nrb->pitch + (X)*nrb->cpp)
#include "spantmp2.h"


/* ARGB8888 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    nouveau##x##_ARGB8888
#define TAG2(x,y) nouveau##x##_ARGB8888##y
#define GET_PTR(X,Y) (map + (Y)*nrb->pitch + (X)*nrb->cpp)
#include "spantmp2.h"

static void
nouveauSpanRenderStart( GLcontext *ctx )
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
   FIRE_RING();
   LOCK_HARDWARE(nmesa);
   nouveauWaitForIdleLocked( nmesa );
}

static void
nouveauSpanRenderFinish( GLcontext *ctx )
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
   _swrast_flush( ctx );
   nouveauWaitForIdleLocked( nmesa );
   UNLOCK_HARDWARE( nmesa );
}

void nouveauSpanInitFunctions( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart	= nouveauSpanRenderStart;
   swdd->SpanRenderFinish	= nouveauSpanRenderFinish;
}


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
nouveauSpanSetFunctions(nouveau_renderbuffer *nrb, const GLvisual *vis)
{
   if (nrb->mesa._ActualFormat == GL_RGBA8)
      nouveauInitPointers_ARGB8888(&nrb->mesa);
   else if (nrb->mesa._ActualFormat == GL_RGB5)
      nouveauInitPointers_RGB565(&nrb->mesa);
}
