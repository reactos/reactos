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
   i810ScreenPrivate *i810Screen = imesa->i810Screen;	\
   GLuint pitch = i810Screen->backPitch;		\
   GLuint height = dPriv->h;				\
   GLushort p;						\
   char *buf = (char *)(imesa->drawMap +		\
			dPriv->x * 2 +			\
			dPriv->y * pitch);		\
   char *read_buf = (char *)(imesa->readMap +		\
			     dPriv->x * 2 +		\
			     dPriv->y * pitch);		\
   (void) read_buf; (void) buf; (void) p

#define LOCAL_DEPTH_VARS				\
   i810ContextPtr imesa = I810_CONTEXT(ctx);	        \
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;	\
   i810ScreenPrivate *i810Screen = imesa->i810Screen;	\
   GLuint pitch = i810Screen->backPitch;		\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(i810Screen->depth.map +	\
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
   GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch);		\
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

#define TAG(x) i810##x##_16
#include "depthtmp.h"


/*
 * This function is called to specify which buffer to read and write
 * for software rasterization (swrast) fallbacks.  This doesn't necessarily
 * correspond to glDrawBuffer() or glReadBuffer() calls.
 */
static void i810SetBuffer(GLcontext *ctx, GLframebuffer *buffer,
                          GLuint bufferBit )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   (void) buffer;

   switch(bufferBit) {
    case BUFFER_BIT_FRONT_LEFT:
      if ( imesa->sarea->pf_current_page == 1)
        imesa->readMap = imesa->i810Screen->back.map;
      else
        imesa->readMap = (char*)imesa->driScreen->pFB;
      break;
    case BUFFER_BIT_BACK_LEFT:
      if ( imesa->sarea->pf_current_page == 1)
        imesa->readMap =  (char*)imesa->driScreen->pFB;
      else
        imesa->readMap = imesa->i810Screen->back.map;
      break;
    default:
      	ASSERT(0);
	break;
   }
   imesa->drawMap = imesa->readMap;
}

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

   swdd->SetBuffer = i810SetBuffer;

#if 0
   swdd->WriteRGBASpan = i810WriteRGBASpan_565;
   swdd->WriteRGBSpan = i810WriteRGBSpan_565;
   swdd->WriteMonoRGBASpan = i810WriteMonoRGBASpan_565;
   swdd->WriteRGBAPixels = i810WriteRGBAPixels_565;
   swdd->WriteMonoRGBAPixels = i810WriteMonoRGBAPixels_565;
   swdd->ReadRGBASpan = i810ReadRGBASpan_565;
   swdd->ReadRGBAPixels = i810ReadRGBAPixels_565;
#endif

#if 0
   swdd->ReadDepthSpan = i810ReadDepthSpan_16;
   swdd->WriteDepthSpan = i810WriteDepthSpan_16;
   swdd->ReadDepthPixels = i810ReadDepthPixels_16;
   swdd->WriteDepthPixels = i810WriteDepthPixels_16;
#endif

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
      drb->Base.GetRow        = i810ReadRGBASpan_565;
      drb->Base.GetValues     = i810ReadRGBAPixels_565;
      drb->Base.PutRow        = i810WriteRGBASpan_565;
      drb->Base.PutRowRGB     = i810WriteRGBSpan_565;
      drb->Base.PutMonoRow    = i810WriteMonoRGBASpan_565;
      drb->Base.PutValues     = i810WriteRGBAPixels_565;
      drb->Base.PutMonoValues = i810WriteMonoRGBAPixels_565;
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      drb->Base.GetRow        = i810ReadDepthSpan_16;
      drb->Base.GetValues     = i810ReadDepthPixels_16;
      drb->Base.PutRow        = i810WriteDepthSpan_16;
      drb->Base.PutMonoRow    = i810WriteMonoDepthSpan_16;
      drb->Base.PutValues     = i810WriteDepthPixels_16;
      drb->Base.PutMonoValues = NULL;
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
