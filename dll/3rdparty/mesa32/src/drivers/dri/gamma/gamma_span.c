/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_span.c,v 1.4 2002/11/05 17:46:07 tsi Exp $ */

#include "gamma_context.h"
#include "gamma_lock.h"
#include "colormac.h"

#include "swrast/swrast.h"

#define DBG 0

#define LOCAL_VARS							\
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);				\
   gammaScreenPtr gammascrn = gmesa->gammaScreen;			\
   __DRIscreenPrivate *sPriv = gmesa->driScreen;			\
   __DRIdrawablePrivate *dPriv = gmesa->driDrawable;			\
   GLuint pitch = sPriv->fbWidth * gammascrn->cpp;		\
   GLuint height = dPriv->h;						\
   char *buf = (char *)(sPriv->pFB +					\
			gmesa->drawOffset +				\
			(dPriv->x * gammascrn->cpp) +			\
			(dPriv->y * pitch));				\
   GLuint p;								\
   (void) buf; (void) p

/* FIXME! Depth/Stencil read/writes don't work ! */
#define LOCAL_DEPTH_VARS				\
   gammaScreenPtr gammascrn = gmesa->gammaScreen;	\
   __DRIdrawablePrivate *dPriv = gmesa->driDrawable;	\
   __DRIscreenPrivate *sPriv = gmesa->driScreen;	\
   GLuint pitch = gammascrn->depthPitch;		\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(sPriv->pFB +			\
			gammascrn->depthOffset +	\
			dPriv->x * gammascrn->cpp +	\
			dPriv->y * pitch)

#define LOCAL_STENCIL_VARS	LOCAL_DEPTH_VARS

#define Y_FLIP( _y )		(height - _y - 1)

#define HW_LOCK()							\
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);				\
   FLUSH_DMA_BUFFER(gmesa);						\
   gammaGetLock( gmesa, DRM_LOCK_FLUSH | DRM_LOCK_QUIESCENT );		\
   GAMMAHW_LOCK( gmesa );

#define HW_UNLOCK() GAMMAHW_UNLOCK( gmesa )



/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_565( color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)r & 0xf8) << 8) |	\
					   (((int)g & 0xfc) << 3) |	\
					   (((int)b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )					\
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
   do {									\
      GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);		\
      rgba[0] = (p >> 8) & 0xf8;					\
      rgba[1] = (p >> 3) & 0xfc;					\
      rgba[2] = (p << 3) & 0xf8;					\
      rgba[3] = 0xff;							\
      if ( rgba[0] & 0x08 ) rgba[0] |= 0x07;				\
      if ( rgba[1] & 0x04 ) rgba[1] |= 0x03;				\
      if ( rgba[2] & 0x08 ) rgba[2] |= 0x07;				\
   } while (0)

#define TAG(x) gamma##x##_RGB565
#include "spantmp.h"


/* 32 bit, ARGB8888 color spanline and pixel functions
 */

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_8888( color[3], color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLuint *)(buf + _x*4 + _y*pitch) = ((b <<  0) |			\
					 (g <<  8) |			\
					 (r << 16) |			\
					 (a << 24) )

#define WRITE_PIXEL( _x, _y, p )					\
   *(GLuint *)(buf + _x*4 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )					\
do {									\
   GLuint p = *(GLuint *)(buf + _x*4 + _y*pitch);			\
   rgba[0] = (p >> 16) & 0xff;						\
   rgba[1] = (p >>  8) & 0xff;						\
   rgba[2] = (p >>  0) & 0xff;						\
   rgba[3] = (p >> 24) & 0xff;						\
} while (0)

#define TAG(x) gamma##x##_ARGB8888
#include "spantmp.h"


/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLushort *)(buf + (_x)*2 + (_y)*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLushort *)(buf + (_x)*2 + (_y)*pitch);	

#define TAG(x) gamma##x##_16
#include "depthtmp.h"


#if 0 /* Unused */
/* 32 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	

#define TAG(x) gamma##x##_32
#include "depthtmp.h"
#endif


/* 24/8 bit interleaved depth/stencil functions
 */
#define WRITE_DEPTH( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + (_x)*4 + (_y)*pitch);	\
   tmp &= 0xff;						\
   tmp |= (d) & 0xffffff00;				\
   *(GLuint *)(buf + (_x)*4 + (_y)*pitch) = tmp;		\
}

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + (_x)*4 + (_y)*pitch) & ~0xff;	


#define TAG(x) gamma##x##_24_8
#include "depthtmp.h"

#if 0
#define WRITE_STENCIL( _x, _y, d ) {			\
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xffffff00;					\
   tmp |= d & 0xff;					\
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp;		\
}

#define READ_STENCIL( d, _x, _y )		\
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & 0xff;	

#define TAG(x) gamma##x##_24_8
#include "stenciltmp.h"

static void gammaReadRGBASpan8888( const GLcontext *ctx,
			       GLuint n, GLint x, GLint y,
			       GLubyte rgba[][4])
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   gammaScreenPtr gammascrn = gmesa->gammaScreen;
   u_int32_t dwords1, dwords2, i = 0;
   char *src = (char *)rgba[0];
   GLuint read = n * gammascrn->cpp; /* Number of bytes we are expecting */
   u_int32_t data;

   FLUSH_DMA_BUFFER(gmesa);
   CHECK_DMA_BUFFER(gmesa, 16);
   WRITE(gmesa->buf, LBReadMode, gmesa->LBReadMode & ~(LBReadSrcEnable | LBReadDstEnable));
   WRITE(gmesa->buf, ColorDDAMode, ColorDDAEnable);
   WRITE(gmesa->buf, LBWriteMode, LBWriteModeDisable);
   WRITE(gmesa->buf, FBReadMode, (gmesa->FBReadMode & ~FBReadSrcEnable) | FBReadDstEnable | FBDataTypeColor);
   WRITE(gmesa->buf, FilterMode, 0x200); /* Pass FBColorData */
   WRITE(gmesa->buf, FBWriteMode, FBW_UploadColorData | FBWriteModeDisable);
   WRITE(gmesa->buf, StartXSub, (x+n)<<16);
   WRITE(gmesa->buf, StartXDom, x<<16);
   WRITE(gmesa->buf, StartY, y<<16);
   WRITE(gmesa->buf, GLINTCount, 1);
   WRITE(gmesa->buf, dXDom, 0<<16);
   WRITE(gmesa->buf, dXSub, 0<<16);
   WRITE(gmesa->buf, dY, 1<<16);
   WRITE(gmesa->buf, Render, PrimitiveTrapezoid);
   FLUSH_DMA_BUFFER(gmesa);

moredata:

   dwords1 = *(volatile u_int32_t*)(void *)(((u_int8_t*)gammascrn->regions[0].map) + (GlintOutFIFOWords));
   dwords2 = *(volatile u_int32_t*)(void *)(((u_int8_t*)gammascrn->regions[2].map) + (GlintOutFIFOWords));

   if (dwords1) {
	memcpy(src, (char*)gammascrn->regions[1].map + 0x1000, dwords1 << 2);
	src += dwords1 << 2;
	read -= dwords1 << 2;
   }
   if (dwords2) {
	memcpy(src, (char*)gammascrn->regions[3].map + 0x1000, dwords2 << 2);
	src += dwords2 << 2;
	read -= dwords2 << 2;
   }

   if (read)
	goto moredata;

done:

   CHECK_DMA_BUFFER(gmesa, 6);
   WRITE(gmesa->buf, ColorDDAMode, gmesa->ColorDDAMode);
   WRITE(gmesa->buf, LBWriteMode, LBWriteModeEnable);
   WRITE(gmesa->buf, LBReadMode, gmesa->LBReadMode);
   WRITE(gmesa->buf, FBReadMode, gmesa->FBReadMode);
   WRITE(gmesa->buf, FBWriteMode, FBWriteModeEnable);
   WRITE(gmesa->buf, FilterMode, 0x400);
}
#endif

static void gammaSetBuffer( GLcontext *ctx,
                            GLframebuffer *colorBuffer,
                            GLuint bufferBit )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   switch ( bufferBit ) {
   case BUFFER_BIT_FRONT_LEFT:
      gmesa->readOffset = 0;
      break;
   case BUFFER_BIT_BACK_LEFT:
      gmesa->readOffset = gmesa->driScreen->fbHeight * gmesa->driScreen->fbWidth * gmesa->gammaScreen->cpp; 
      break;
   default:
      _mesa_problem(ctx, "Unexpected buffer 0x%x in gammaSetBuffer()", bufferBit);
   }
}


void gammaDDInitSpanFuncs( GLcontext *ctx )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = gammaSetBuffer;

   switch ( gmesa->gammaScreen->cpp ) {
   case 2:
      swdd->WriteRGBASpan	= gammaWriteRGBASpan_RGB565;
      swdd->WriteRGBSpan	= gammaWriteRGBSpan_RGB565;
      swdd->WriteMonoRGBASpan	= gammaWriteMonoRGBASpan_RGB565;
      swdd->WriteRGBAPixels	= gammaWriteRGBAPixels_RGB565;
      swdd->WriteMonoRGBAPixels	= gammaWriteMonoRGBAPixels_RGB565;
      swdd->ReadRGBASpan	= gammaReadRGBASpan_RGB565;
      swdd->ReadRGBAPixels      = gammaReadRGBAPixels_RGB565;
      break;

   case 4:
      swdd->WriteRGBASpan	= gammaWriteRGBASpan_ARGB8888;
      swdd->WriteRGBSpan	= gammaWriteRGBSpan_ARGB8888;
      swdd->WriteMonoRGBASpan   = gammaWriteMonoRGBASpan_ARGB8888;
      swdd->WriteRGBAPixels     = gammaWriteRGBAPixels_ARGB8888;
      swdd->WriteMonoRGBAPixels = gammaWriteMonoRGBAPixels_ARGB8888;
#if 1
      swdd->ReadRGBASpan	= gammaReadRGBASpan_ARGB8888;
#else
      swdd->ReadRGBASpan	= gammaReadRGBASpan8888;
#endif
      swdd->ReadRGBAPixels      = gammaReadRGBAPixels_ARGB8888;
      break;

   default:
      break;
   }

   switch ( gmesa->glCtx->Visual.depthBits ) {
   case 16:
      swdd->ReadDepthSpan	= gammaReadDepthSpan_16;
      swdd->WriteDepthSpan	= gammaWriteDepthSpan_16;
      swdd->ReadDepthPixels	= gammaReadDepthPixels_16;
      swdd->WriteDepthPixels	= gammaWriteDepthPixels_16;
      break;

   case 24:
      swdd->ReadDepthSpan	= gammaReadDepthSpan_24_8;
      swdd->WriteDepthSpan	= gammaWriteDepthSpan_24_8;
      swdd->ReadDepthPixels	= gammaReadDepthPixels_24_8;
      swdd->WriteDepthPixels	= gammaWriteDepthPixels_24_8;

#if 0
      swdd->ReadStencilSpan	= gammaReadStencilSpan_24_8;
      swdd->WriteStencilSpan	= gammaWriteStencilSpan_24_8;
      swdd->ReadStencilPixels	= gammaReadStencilPixels_24_8;
      swdd->WriteStencilPixels	= gammaWriteStencilPixels_24_8;
#endif
      break;

   default:
      break;
   }
}
