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

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "colormac.h"

#include "intel_fbo.h"
#include "intel_screen.h"
#include "intel_span.h"
#include "intel_regions.h"
#include "intel_ioctl.h"
#include "intel_tex.h"

#include "swrast/swrast.h"

/*
  break intelWriteRGBASpan_ARGB8888
*/

#undef DBG
#define DBG 0

#define LOCAL_VARS							\
   struct intel_context *intel = intel_context(ctx);			\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   const GLint yScale = irb->RenderToTexture ? 1 : -1;			\
   const GLint yBias = irb->RenderToTexture ? 0 : irb->Base.Height - 1;	\
   GLubyte *buf = (GLubyte *) irb->pfMap				\
      + (intel->drawY * irb->pfPitch + intel->drawX) * irb->region->cpp;\
   GLuint p;								\
   assert(irb->pfMap);\
   (void) p;

/* XXX FBO: this is identical to the macro in spantmp2.h except we get
 * the cliprect info from the context, not the driDrawable.
 * Move this into spantmp2.h someday.
 */
#define HW_CLIPLOOP()							\
   do {									\
      int _nc = intel->numClipRects;					\
      while ( _nc-- ) {							\
	 int minx = intel->pClipRects[_nc].x1 - intel->drawX;		\
	 int miny = intel->pClipRects[_nc].y1 - intel->drawY;		\
	 int maxx = intel->pClipRects[_nc].x2 - intel->drawX;		\
	 int maxy = intel->pClipRects[_nc].y2 - intel->drawY;




#define Y_FLIP(_y) ((_y) * yScale + yBias)

#define HW_LOCK()

#define HW_UNLOCK()

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    intel##x##_RGB565
#define TAG2(x,y) intel##x##_RGB565##y
#define GET_PTR(X,Y) (buf + ((Y) * irb->pfPitch + (X)) * 2)
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    intel##x##_ARGB8888
#define TAG2(x,y) intel##x##_ARGB8888##y
#define GET_PTR(X,Y) (buf + ((Y) * irb->pfPitch + (X)) * 4)
#include "spantmp2.h"


#define LOCAL_DEPTH_VARS						\
   struct intel_context *intel = intel_context(ctx);			\
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);		\
   const GLuint pitch = irb->pfPitch/***XXX region->pitch*/; /* in pixels */ \
   const GLint yScale = irb->RenderToTexture ? 1 : -1;			\
   const GLint yBias = irb->RenderToTexture ? 0 : irb->Base.Height - 1;	\
   char *buf = (char *) irb->pfMap/*XXX use region->map*/ +             \
      (intel->drawY * pitch + intel->drawX) * irb->region->cpp;


#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS

/**
 ** 16-bit depthbuffer functions.
 **/
#define WRITE_DEPTH( _x, _y, d ) \
   ((GLushort *)buf)[(_x) + (_y) * pitch] = d;

#define READ_DEPTH( d, _x, _y )	\
   d = ((GLushort *)buf)[(_x) + (_y) * pitch];


#define TAG(x) intel##x##_z16
#include "depthtmp.h"


/**
 ** 24/8-bit interleaved depth/stencil functions
 ** Note: we're actually reading back combined depth+stencil values.
 ** The wrappers in main/depthstencil.c are used to extract the depth
 ** and stencil values.
 **/
/* Change ZZZS -> SZZZ */
#define WRITE_DEPTH( _x, _y, d ) {				\
   GLuint tmp = ((d) >> 8) | ((d) << 24);			\
   ((GLuint *)buf)[(_x) + (_y) * pitch] = tmp;			\
}

/* Change SZZZ -> ZZZS */
#define READ_DEPTH( d, _x, _y ) {				\
   GLuint tmp = ((GLuint *)buf)[(_x) + (_y) * pitch];		\
   d = (tmp << 8) | (tmp >> 24);				\
}

#define TAG(x) intel##x##_z24_s8
#include "depthtmp.h"


/**
 ** 8-bit stencil function (XXX FBO: This is obsolete)
 **/
#define WRITE_STENCIL( _x, _y, d ) {				\
   GLuint tmp = ((GLuint *)buf)[(_x) + (_y) * pitch];		\
   tmp &= 0xffffff;						\
   tmp |= ((d) << 24);						\
   ((GLuint *) buf)[(_x) + (_y) * pitch] = tmp;			\
}

#define READ_STENCIL( d, _x, _y )				\
   d = ((GLuint *)buf)[(_x) + (_y) * pitch] >> 24;

#define TAG(x) intel##x##_z24_s8
#include "stenciltmp.h"



/**
 * Map or unmap all the renderbuffers which we may need during
 * software rendering.
 * XXX in the future, we could probably convey extra information to
 * reduce the number of mappings needed.  I.e. if doing a glReadPixels
 * from the depth buffer, we really only need one mapping.
 *
 * XXX Rewrite this function someday.
 * We can probably just loop over all the renderbuffer attachments,
 * map/unmap all of them, and not worry about the _ColorDrawBuffers
 * _ColorReadBuffer, _DepthBuffer or _StencilBuffer fields.
 */
static void
intel_map_unmap_buffers(struct intel_context *intel, GLboolean map)
{
   GLcontext *ctx = &intel->ctx;
   GLuint i, j;
   struct intel_renderbuffer *irb;

   /* color draw buffers */
   for (i = 0; i < ctx->Const.MaxDrawBuffers; i++) {
      for (j = 0; j < ctx->DrawBuffer->_NumColorDrawBuffers[i]; j++) {
         struct gl_renderbuffer *rb =
            ctx->DrawBuffer->_ColorDrawBuffers[i][j];
         irb = intel_renderbuffer(rb);
         if (irb) {
            /* this is a user-created intel_renderbuffer */
            if (irb->region) {
               if (map)
                  intel_region_map(intel->intelScreen, irb->region);
               else
                  intel_region_unmap(intel->intelScreen, irb->region);
            }
            irb->pfMap = irb->region->map;
            irb->pfPitch = irb->region->pitch;
         }
      }
   }

   /* check for render to textures */
   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att =
         ctx->DrawBuffer->Attachment + i;
      struct gl_texture_object *tex = att->Texture;
      if (tex) {
         /* render to texture */
         ASSERT(att->Renderbuffer);
         if (map) {
            struct gl_texture_image *texImg;
            texImg = tex->Image[att->CubeMapFace][att->TextureLevel];
            intel_tex_map_images(intel, intel_texture_object(tex));
         }
         else {
            intel_tex_unmap_images(intel, intel_texture_object(tex));
         }
      }
   }

   /* color read buffers */
   irb = intel_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
   if (irb && irb->region) {
      if (map)
         intel_region_map(intel->intelScreen, irb->region);
      else
         intel_region_unmap(intel->intelScreen, irb->region);
      irb->pfMap = irb->region->map;
      irb->pfPitch = irb->region->pitch;
   }

   /* Account for front/back color page flipping.
    * The span routines use the pfMap and pfPitch fields which will
    * swap the front/back region map/pitch if we're page flipped.
    * Do this after mapping, above, so the map field is valid.
    */
#if 0
   if (map && ctx->DrawBuffer->Name == 0) {
      struct intel_renderbuffer *irbFront
         = intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_FRONT_LEFT);
      struct intel_renderbuffer *irbBack
         = intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_BACK_LEFT);
      if (irbBack) {
         /* double buffered */
         if (intel->sarea->pf_current_page == 0) {
            irbFront->pfMap = irbFront->region->map;
            irbFront->pfPitch = irbFront->region->pitch;
            irbBack->pfMap = irbBack->region->map;
            irbBack->pfPitch = irbBack->region->pitch;
         }
         else {
            irbFront->pfMap = irbBack->region->map;
            irbFront->pfPitch = irbBack->region->pitch;
            irbBack->pfMap = irbFront->region->map;
            irbBack->pfPitch = irbFront->region->pitch;
         }
      }
   }
#endif

   /* depth buffer (Note wrapper!) */
   if (ctx->DrawBuffer->_DepthBuffer) {
      irb = intel_renderbuffer(ctx->DrawBuffer->_DepthBuffer->Wrapped);
      if (irb && irb->region && irb->Base.Name != 0) {
         if (map) {
            intel_region_map(intel->intelScreen, irb->region);
            irb->pfMap = irb->region->map;
            irb->pfPitch = irb->region->pitch;
         }
         else {
            intel_region_unmap(intel->intelScreen, irb->region);
            irb->pfMap = NULL;
            irb->pfPitch = 0;
         }
      }
   }

   /* stencil buffer (Note wrapper!) */
   if (ctx->DrawBuffer->_StencilBuffer) {
      irb = intel_renderbuffer(ctx->DrawBuffer->_StencilBuffer->Wrapped);
      if (irb && irb->region && irb->Base.Name != 0) {
         if (map) {
            intel_region_map(intel->intelScreen, irb->region);
            irb->pfMap = irb->region->map;
            irb->pfPitch = irb->region->pitch;
         }
         else {
            intel_region_unmap(intel->intelScreen, irb->region);
            irb->pfMap = NULL;
            irb->pfPitch = 0;
         }
      }
   }
}



/**
 * Prepare for softare rendering.  Map current read/draw framebuffers'
 * renderbuffes and all currently bound texture objects.
 *
 * Old note: Moved locking out to get reasonable span performance.
 */
void
intelSpanRenderStart(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);
   GLuint i;

   intelFinish(&intel->ctx);
   LOCK_HARDWARE(intel);

#if 0
   /* Just map the framebuffer and all textures.  Bufmgr code will
    * take care of waiting on the necessary fences:
    */
   intel_region_map(intel->intelScreen, intel->front_region);
   intel_region_map(intel->intelScreen, intel->back_region);
   intel_region_map(intel->intelScreen, intel->intelScreen->depth_region);
#endif

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;
         intel_tex_map_images(intel, intel_texture_object(texObj));
      }
   }

   intel_map_unmap_buffers(intel, GL_TRUE);
}

/**
 * Called when done softare rendering.  Unmap the buffers we mapped in
 * the above function.
 */
void
intelSpanRenderFinish(GLcontext * ctx)
{
   struct intel_context *intel = intel_context(ctx);
   GLuint i;

   _swrast_flush(ctx);

   /* Now unmap the framebuffer:
    */
#if 0
   intel_region_unmap(intel, intel->front_region);
   intel_region_unmap(intel, intel->back_region);
   intel_region_unmap(intel, intel->intelScreen->depth_region);
#endif

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
         struct gl_texture_object *texObj = ctx->Texture.Unit[i]._Current;
         intel_tex_unmap_images(intel, intel_texture_object(texObj));
      }
   }

   intel_map_unmap_buffers(intel, GL_FALSE);

   UNLOCK_HARDWARE(intel);
}


void
intelInitSpanFuncs(GLcontext * ctx)
{
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);
   swdd->SpanRenderStart = intelSpanRenderStart;
   swdd->SpanRenderFinish = intelSpanRenderFinish;
}


/**
 * Plug in appropriate span read/write functions for the given renderbuffer.
 * These are used for the software fallbacks.
 */
void
intel_set_span_functions(struct gl_renderbuffer *rb)
{
   if (rb->_ActualFormat == GL_RGB5) {
      /* 565 RGB */
      intelInitPointers_RGB565(rb);
   }
   else if (rb->_ActualFormat == GL_RGBA8) {
      /* 8888 RGBA */
      intelInitPointers_ARGB8888(rb);
   }
   else if (rb->_ActualFormat == GL_DEPTH_COMPONENT16) {
      intelInitDepthPointers_z16(rb);
   }
   else if (rb->_ActualFormat == GL_DEPTH_COMPONENT24 ||        /* XXX FBO remove */
            rb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT) {
      intelInitDepthPointers_z24_s8(rb);
   }
   else if (rb->_ActualFormat == GL_STENCIL_INDEX8_EXT) {       /* XXX FBO remove */
      intelInitStencilPointers_z24_s8(rb);
   }
   else {
      _mesa_problem(NULL,
                    "Unexpected _ActualFormat in intelSetSpanFunctions");
   }
}
