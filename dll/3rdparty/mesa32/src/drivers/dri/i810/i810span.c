#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "colormac.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810span.h"
#include "i810ioctl.h"
#include "swrast/swrast.h"


#define DBG 0

#define LOCAL_VARS					\
   i810ContextPtr imesa = I810_CONTEXT(ctx);	        \
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;	\
   driRenderbuffer *drb = (driRenderbuffer *) rb;	\
   GLuint pitch = drb->pitch;				\
   GLuint height = dPriv->h;				\
   GLushort p;						\
   char *buf = (char *)(drb->flippedData +		\
			dPriv->x * 2 +			\
			dPriv->y * pitch);		\
   (void) buf; (void) p

#define LOCAL_DEPTH_VARS				\
   i810ContextPtr imesa = I810_CONTEXT(ctx);	        \
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;	\
   driRenderbuffer *drb = (driRenderbuffer *) rb;	\
   GLuint pitch = drb->pitch;				\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(drb->Base.Data +		\
			dPriv->x * 2 +			\
			dPriv->y * pitch)

#define INIT_MONO_PIXEL(p, color) \
   p = PACK_COLOR_565( color[0], color[1], color[2] )

#define Y_FLIP(_y) (height - _y - 1)

#define HW_LOCK()

#define HW_UNLOCK()

/* 16 bit, 565 rgb color spanline and pixel functions
 */
#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( (((int)r & 0xf8) << 8) |	\
		                             (((int)g & 0xfc) << 3) |	\
		                             (((int)b & 0xf8) >> 3))
#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
do {									\
   GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);			\
   rgba[0] = ((p >> 8) & 0xf8) * 255 / 0xf8;				\
   rgba[1] = ((p >> 3) & 0xfc) * 255 / 0xfc;				\
   rgba[2] = ((p << 3) & 0xf8) * 255 / 0xf8;				\
   rgba[3] = 255;							\
} while(0)

#define TAG(x) i810##x##_565
#include "spantmp.h"

/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d ) \
   *(GLushort *)(buf + (_x)*2 + (_y)*pitch)  = d;

#define READ_DEPTH( d, _x, _y )	\
   d = *(GLushort *)(buf + (_x)*2 + (_y)*pitch);

#define TAG(x) i810##x##_z16
#include "depthtmp.h"


/* Move locking out to get reasonable span performance.
 */
void i810SpanRenderStart( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   I810_FIREVERTICES(imesa);
   LOCK_HARDWARE(imesa);
   i810RegetLockQuiescent( imesa );
}

void i810SpanRenderFinish( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   _swrast_flush( ctx );
   UNLOCK_HARDWARE( imesa );
}

void i810InitSpanFuncs( GLcontext *ctx )
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart = i810SpanRenderStart;
   swdd->SpanRenderFinish = i810SpanRenderFinish; 
}



/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
i810SetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      /* always 565 RGB */
      i810InitPointers_565(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      i810InitDepthPointers_z16(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      /* should never get here */
      drb->Base.GetRow        = NULL;
      drb->Base.GetValues     = NULL;
      drb->Base.PutRow        = NULL;
      drb->Base.PutMonoRow    = NULL;
      drb->Base.PutValues     = NULL;
      drb->Base.PutMonoValues = NULL;
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      drb->Base.GetRow        = NULL;
      drb->Base.GetValues     = NULL;
      drb->Base.PutRow        = NULL;
      drb->Base.PutMonoRow    = NULL;
      drb->Base.PutValues     = NULL;
      drb->Base.PutMonoValues = NULL;
   }
}
