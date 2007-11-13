/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "s3v_context.h"
#include "s3v_lock.h"

#include "swrast/swrast.h"

#define _SPANLOCK 1
#define DBG 0

#define LOCAL_VARS \
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);				\
	__DRIscreenPrivate *sPriv = vmesa->driScreen;			\
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;		\
	driRenderbuffer *drb = (driRenderbuffer *) rb;			\
	GLuint cpp = drb->cpp;						\
	GLuint pitch = ( (drb->backBuffer) ?				\
			((dPriv->w+31)&~31) * cpp			\
			: sPriv->fbWidth * cpp);			\
	GLuint height = dPriv->h;					\
	char *buf = (char *)(sPriv->pFB + drb->offset			\
	   + (drb->backBuffer ?	0 : dPriv->x * cpp + dPriv->y * pitch));\
	GLuint p; \
	(void) p

/* FIXME! Depth/Stencil read/writes don't work ! */
#define LOCAL_DEPTH_VARS					\
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;	\
	__DRIscreenPrivate *sPriv = vmesa->driScreen;		\
	driRenderbuffer *drb = (driRenderbuffer *) rb;		\
	GLuint pitch = drb->pitch;				\
	GLuint height = dPriv->h;				\
	char *buf = (char *)(sPriv->pFB + drb->offset);		\
	(void) pitch

#define LOCAL_STENCIL_VARS	LOCAL_DEPTH_VARS

#define Y_FLIP( _y )	(height - _y - 1)

#if _SPANLOCK	/* OK, we lock */

#define HW_LOCK() \
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);	\
	(void) vmesa; \
	DMAFLUSH(); \
	S3V_SIMPLE_FLUSH_LOCK(vmesa);
#define HW_UNLOCK() S3V_SIMPLE_UNLOCK(vmesa);

#else			/* plz, don't lock */

#define HW_LOCK() \
	s3vContextPtr vmesa = S3V_CONTEXT(ctx); \
    (void) vmesa; \
	DMAFLUSH(); 
#define HW_UNLOCK()

#endif


/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define INIT_MONO_PIXEL(p, color) \
  p = S3VIRGEPACKCOLOR555( color[0], color[1], color[2], color[3] )

#define WRITE_RGBA( _x, _y, r, g, b, a ) \
do { \
   *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)r & 0xf8) << 7) | \
					   (((int)g & 0xf8) << 2) | \
					   (((int)b & 0xf8) >> 3)); \
   DEBUG(("buf=0x%x drawOffset=0x%x dPriv->x=%i drb->cpp=%i dPriv->y=%i pitch=%i\n", \
   	sPriv->pFB, vmesa->drawOffset, dPriv->x, drb->cpp, dPriv->y, pitch)); \
   DEBUG(("dPriv->w = %i\n", dPriv->w)); \
} while(0)

#define WRITE_PIXEL( _x, _y, p ) \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y ) \
   do { \
      GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch); \
      rgba[0] = (p >> 7) & 0xf8; \
      rgba[1] = (p >> 2) & 0xf8; \
      rgba[2] = (p << 3) & 0xf8; \
      rgba[3] = 0xff; /*
      if ( rgba[0] & 0x08 ) rgba[0] |= 0x07; \ 
      if ( rgba[1] & 0x04 ) rgba[1] |= 0x03; \
      if ( rgba[2] & 0x08 ) rgba[2] |= 0x07; */ \
   } while (0)

#define TAG(x) s3v##x##_RGB555
#include "spantmp.h"


/* 32 bit, ARGB8888 color spanline and pixel functions
 */

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_8888( color[3], color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a ) \
   *(GLuint *)(buf + _x*4 + _y*pitch) = ((b <<  0) | \
					 (g <<  8) | \
					 (r << 16) | \
					 (a << 24) )

#define WRITE_PIXEL( _x, _y, p ) \
   *(GLuint *)(buf + _x*4 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y ) \
do { \
   GLuint p = *(GLuint *)(buf + _x*4 + _y*pitch); \
   rgba[0] = (p >> 16) & 0xff; \
   rgba[1] = (p >>  8) & 0xff; \
   rgba[2] = (p >>  0) & 0xff; \
   rgba[3] = (p >> 24) & 0xff; \
} while (0)

#define TAG(x) s3v##x##_ARGB8888
#include "spantmp.h"


/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d ) \
   *(GLushort *)(buf + _x*2 + _y*dPriv->w*2) = d

#define READ_DEPTH( d, _x, _y ) \
   d = *(GLushort *)(buf + _x*2 + _y*dPriv->w*2);

#define TAG(x) s3v##x##_z16
#include "depthtmp.h"




/* 32 bit depthbuffer functions.
 */
#if 0
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLuint *)(buf + _x*4 + _y*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + _x*4 + _y*pitch);	

#define TAG(x) s3v##x##_32
#include "depthtmp.h"
#endif


/* 24/8 bit interleaved depth/stencil functions
 */
#if 0
#define WRITE_DEPTH( _x, _y, d ) { \
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xff; \
   tmp |= (d) & 0xffffff00; \
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp; \
}

#define READ_DEPTH( d, _x, _y ) \
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & ~0xff	


#define TAG(x) s3v##x##_24_8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) { \
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xffffff00; \
   tmp |= d & 0xff; \
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp; \
}

#define READ_STENCIL( d, _x, _y ) \
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & 0xff	

#define TAG(x) s3v##x##_24_8
#include "stenciltmp.h"

#endif


/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
s3vSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         s3vInitPointers_RGB555(&drb->Base);
      }
      else {
         s3vInitPointers_ARGB8888(&drb->Base);
      }
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT16) {
      s3vInitDepthPointers_z16(&drb->Base);
   }
   else if (drb->Base.InternalFormat == GL_DEPTH_COMPONENT24) {
      /* not done yet */
   }
   else if (drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
      /* not done yet */
   }
}
