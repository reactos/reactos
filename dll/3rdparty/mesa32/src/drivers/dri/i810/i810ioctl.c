/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810ioctl.c,v 1.7 2002/10/30 12:51:33 alanh Exp $ */

#include <unistd.h> /* for usleep() */

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "swrast/swrast.h"
#include "mm.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810context.h"
#include "i810ioctl.h"
#include "i810state.h"

static drmBufPtr i810_get_buffer_ioctl( i810ContextPtr imesa )
{
   drmI810DMA dma;
   drmBufPtr buf;
   int retcode, i = 0;
   
   while (1) {
      retcode = drmCommandWriteRead(imesa->driFd, DRM_I810_GETBUF,
                                    &dma, sizeof(drmI810DMA));

      if (dma.granted == 1 && retcode == 0) 
	 break;
      
      if (++i > 1000) {
	 drmCommandNone(imesa->driFd, DRM_I810_FLUSH);
	 i = 0;
      }
   }

   buf = &(imesa->i810Screen->bufs->list[dma.request_idx]);
   buf->idx = dma.request_idx;
   buf->used = 0;
   buf->total = dma.request_size;
   buf->address = (drmAddress)dma.virtual;

   return buf;
}



#define DEPTH_SCALE ((1<<16)-1)

static void i810Clear( GLcontext *ctx, GLbitfield mask )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
   drmI810Clear clear;
   unsigned int i;

   clear.flags = 0;
   clear.clear_color = imesa->ClearColor;
   clear.clear_depth = (GLuint) (ctx->Depth.Clear * DEPTH_SCALE);

   I810_FIREVERTICES( imesa );
	
   if ((mask & BUFFER_BIT_FRONT_LEFT) && colorMask == ~0U) {
      clear.flags |= I810_FRONT;
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if ((mask & BUFFER_BIT_BACK_LEFT) && colorMask == ~0U) {
      clear.flags |= I810_BACK;
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if (mask & BUFFER_BIT_DEPTH) {
      if (ctx->Depth.Mask) 
	 clear.flags |= I810_DEPTH;
      mask &= ~BUFFER_BIT_DEPTH;
   }

   if (clear.flags) {
      GLint cx, cy, cw, ch;

      LOCK_HARDWARE( imesa );

      /* compute region after locking: */
      cx = ctx->DrawBuffer->_Xmin;
      cy = ctx->DrawBuffer->_Ymin;
      cw = ctx->DrawBuffer->_Xmax - cx;
      ch = ctx->DrawBuffer->_Ymax - cy;

      /* flip top to bottom */
      cy = dPriv->h-cy-ch;
      cx += imesa->drawX;
      cy += imesa->drawY;

      for (i = 0 ; i < imesa->numClipRects ; ) 
      { 	 
	 unsigned int nr = MIN2(i + I810_NR_SAREA_CLIPRECTS, imesa->numClipRects);
	 drm_clip_rect_t *box = imesa->pClipRects;	 
	 drm_clip_rect_t *b = (drm_clip_rect_t *)imesa->sarea->boxes;
	 int n = 0;

	 if (cw != dPriv->w || ch != dPriv->h) {
            /* clear sub region */
	    for ( ; i < nr ; i++) {
	       GLint x = box[i].x1;
	       GLint y = box[i].y1;
	       GLint w = box[i].x2 - x;
	       GLint h = box[i].y2 - y;

	       if (x < cx) w -= cx - x, x = cx; 
	       if (y < cy) h -= cy - y, y = cy;
	       if (x + w > cx + cw) w = cx + cw - x;
	       if (y + h > cy + ch) h = cy + ch - y;
	       if (w <= 0) continue;
	       if (h <= 0) continue;

	       b->x1 = x;
	       b->y1 = y;
	       b->x2 = x + w;
	       b->y2 = y + h;
	       b++;
	       n++;
	    }
	 } else {
            /* clear whole buffer */
	    for ( ; i < nr ; i++) {
	       *b++ = box[i];
	       n++;
	    }
	 }

	 imesa->sarea->nbox = n;
         drmCommandWrite(imesa->driFd, DRM_I810_CLEAR,
                         &clear, sizeof(drmI810Clear));
      }

      UNLOCK_HARDWARE( imesa );
      imesa->upload_cliprects = GL_TRUE;
   }

   if (mask) 
      _swrast_Clear( ctx, mask );
}




/*
 * Copy the back buffer to the front buffer. 
 */
void i810CopyBuffer( const __DRIdrawablePrivate *dPriv ) 
{
   i810ContextPtr imesa;
   drm_clip_rect_t *pbox;
   int nbox, i, tmp;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   imesa = (i810ContextPtr) dPriv->driContextPriv->driverPrivate;

   I810_FIREVERTICES( imesa );
   LOCK_HARDWARE( imesa );
   
   pbox = (drm_clip_rect_t *)dPriv->pClipRects;
   nbox = dPriv->numClipRects;

   for (i = 0 ; i < nbox ; )
   {
      int nr = MIN2(i + I810_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
      drm_clip_rect_t *b = (drm_clip_rect_t *)imesa->sarea->boxes;

      imesa->sarea->nbox = nr - i;

      for ( ; i < nr ; i++) 
	 *b++ = pbox[i];

      drmCommandNone(imesa->driFd, DRM_I810_SWAP);
   }

   tmp = GET_ENQUEUE_AGE(imesa);
   UNLOCK_HARDWARE( imesa );

   /* multiarb will suck the life out of the server without this throttle:
    */
   if (GET_DISPATCH_AGE(imesa) < imesa->lastSwap) {
      i810WaitAge(imesa, imesa->lastSwap);
   }

   imesa->lastSwap = tmp;
   imesa->upload_cliprects = GL_TRUE;
}


/*
 * XXX implement when full-screen extension is done.
 */
void i810PageFlip( const __DRIdrawablePrivate *dPriv ) 
{
  i810ContextPtr imesa;
  int tmp, ret;

  assert(dPriv);
  assert(dPriv->driContextPriv);
  assert(dPriv->driContextPriv->driverPrivate);
    
  imesa = (i810ContextPtr) dPriv->driContextPriv->driverPrivate;

  I810_FIREVERTICES( imesa );
  LOCK_HARDWARE( imesa );
  
  if (dPriv->pClipRects) {
     memcpy(&(imesa->sarea->boxes[0]), &(dPriv->pClipRects[0]),
            sizeof(drm_clip_rect_t));
     imesa->sarea->nbox = 1;
  }
  ret = drmCommandNone(imesa->driFd, DRM_I810_FLIP);
  if (ret) {
    fprintf(stderr, "%s: %d\n", __FUNCTION__, ret);
    UNLOCK_HARDWARE( imesa );
    exit(1);
  }

  tmp = GET_ENQUEUE_AGE(imesa);
  UNLOCK_HARDWARE( imesa );
  
   /* multiarb will suck the life out of the server without this throttle:
    */
  if (GET_DISPATCH_AGE(imesa) < imesa->lastSwap) {
    i810WaitAge(imesa, imesa->lastSwap);
   }

  /*  i810SetDrawBuffer( imesa->glCtx, imesa->glCtx->Color.DriverDrawBuffer );*/
  i810DrawBuffer( imesa->glCtx, imesa->glCtx->Color.DrawBuffer[0] );
  imesa->upload_cliprects = GL_TRUE;
  imesa->lastSwap = tmp;
  return;
}


/* This waits for *everybody* to finish rendering -- overkill.
 */
void i810DmaFinish( i810ContextPtr imesa  ) 
{
   I810_FIREVERTICES( imesa );

   LOCK_HARDWARE( imesa );
   i810RegetLockQuiescent( imesa );
   UNLOCK_HARDWARE( imesa );
}


void i810RegetLockQuiescent( i810ContextPtr imesa  ) 
{
   drmUnlock(imesa->driFd, imesa->hHWContext);
   i810GetLock( imesa, DRM_LOCK_QUIESCENT ); 
}

void i810WaitAgeLocked( i810ContextPtr imesa, int age  ) 
{
   int i = 0, j;

   while (++i < 5000) {
      drmCommandNone(imesa->driFd, DRM_I810_GETAGE);
      if (GET_DISPATCH_AGE(imesa) >= age)
	 return;
      for (j = 0 ; j < 1000 ; j++)
	 ;
   }

   drmCommandNone(imesa->driFd, DRM_I810_FLUSH);
}


void i810WaitAge( i810ContextPtr imesa, int age  ) 
{
   int i = 0, j;

   while (++i < 5000) {
      drmCommandNone(imesa->driFd, DRM_I810_GETAGE);
      if (GET_DISPATCH_AGE(imesa) >= age)
	 return;
      for (j = 0 ; j < 1000 ; j++)
	 ;
   }

   i = 0;
   while (++i < 1000) {
      drmCommandNone(imesa->driFd, DRM_I810_GETAGE);
      if (GET_DISPATCH_AGE(imesa) >= age)
	 return;
      usleep(1000);
   }

   LOCK_HARDWARE(imesa);
   drmCommandNone(imesa->driFd, DRM_I810_FLUSH);
   UNLOCK_HARDWARE(imesa);
}




static int intersect_rect( drm_clip_rect_t *out,
                           drm_clip_rect_t *a,
                           drm_clip_rect_t *b )
{
   *out = *a;
   if (b->x1 > out->x1) out->x1 = b->x1;
   if (b->x2 < out->x2) out->x2 = b->x2;
   if (out->x1 >= out->x2) return 0;

   if (b->y1 > out->y1) out->y1 = b->y1;
   if (b->y2 < out->y2) out->y2 = b->y2;
   if (out->y1 >= out->y2) return 0;
   return 1;
}


static void emit_state( i810ContextPtr imesa )
{
   GLuint dirty = imesa->dirty;   
   I810SAREAPtr sarea = imesa->sarea;

   if (dirty & I810_UPLOAD_BUFFERS) {
      memcpy( sarea->BufferState, imesa->BufferSetup, 
	      sizeof(imesa->BufferSetup) );
   }	 

   if (dirty & I810_UPLOAD_CTX) {
      memcpy( sarea->ContextState, imesa->Setup, 
	      sizeof(imesa->Setup) );
   }

   if (dirty & I810_UPLOAD_TEX0) {
      memcpy(sarea->TexState[0], 
	     imesa->CurrentTexObj[0]->Setup,
	     sizeof(imesa->CurrentTexObj[0]->Setup));
   }

   if (dirty & I810_UPLOAD_TEX1) {
      GLuint *setup = sarea->TexState[1];

      memcpy( setup,
	      imesa->CurrentTexObj[1]->Setup,
	      sizeof(imesa->CurrentTexObj[1]->Setup));

      /* Need this for the case where both units are bound to the same
       * texobj.  
       */
      setup[I810_TEXREG_MI1] ^= (MI1_MAP_0 ^ MI1_MAP_1);
      setup[I810_TEXREG_MLC] ^= (MLC_MAP_0 ^ MLC_MAP_1);
      setup[I810_TEXREG_MLL] ^= (MLL_MAP_0 ^ MLL_MAP_1);
      setup[I810_TEXREG_MCS] ^= (MCS_COORD_0 ^ MCS_COORD_1);
      setup[I810_TEXREG_MF]  ^= (MF_MAP_0 ^ MF_MAP_1);
   }
    
   sarea->dirty = dirty;
   imesa->dirty = 0;
}


static void age_imesa( i810ContextPtr imesa, int age )
{
   if (imesa->CurrentTexObj[0]) imesa->CurrentTexObj[0]->base.timestamp = age;
   if (imesa->CurrentTexObj[1]) imesa->CurrentTexObj[1]->base.timestamp = age;
}


void i810FlushPrimsLocked( i810ContextPtr imesa )
{
   drm_clip_rect_t *pbox = imesa->pClipRects;
   int nbox = imesa->numClipRects;
   drmBufPtr buffer = imesa->vertex_buffer;
   I810SAREAPtr sarea = imesa->sarea;
   drmI810Vertex vertex;
   int i;
	  
   if (I810_DEBUG & DEBUG_STATE)
      i810PrintDirty( __FUNCTION__, imesa->dirty );
   
   if (imesa->dirty)
      emit_state( imesa );

   vertex.idx = buffer->idx;
   vertex.used = imesa->vertex_low;
   vertex.discard = 0;
   sarea->vertex_prim = imesa->hw_primitive;

   if (!nbox) {
      vertex.used = 0;
   }
   else if (nbox > I810_NR_SAREA_CLIPRECTS) {      
      imesa->upload_cliprects = GL_TRUE;
   }

   if (!nbox || !imesa->upload_cliprects) 
   {
      if (nbox == 1) 
	 sarea->nbox = 0;
      else
	 sarea->nbox = nbox;

      vertex.discard = 1;	
      drmCommandWrite(imesa->driFd, DRM_I810_VERTEX,
                      &vertex, sizeof(drmI810Vertex));
      age_imesa(imesa, sarea->last_enqueue);
   }  
   else 
   {
      for (i = 0 ; i < nbox ; )
      {
	 int nr = MIN2(i + I810_NR_SAREA_CLIPRECTS, nbox);
	 drm_clip_rect_t *b = (drm_clip_rect_t *)sarea->boxes;

	 if (imesa->scissor) {
	    sarea->nbox = 0;
	 
	    for ( ; i < nr ; i++) {
	       b->x1 = pbox[i].x1 - imesa->drawX;
	       b->y1 = pbox[i].y1 - imesa->drawY;
	       b->x2 = pbox[i].x2 - imesa->drawX;
	       b->y2 = pbox[i].y2 - imesa->drawY;

	       if (intersect_rect(b, b, &imesa->scissor_rect)) {
		  sarea->nbox++;
		  b++;
	       }
	    }

	    /* Culled?
	     */
	    if (!sarea->nbox) {
	       if (nr < nbox) continue;
	       vertex.used = 0;
	    }
	 } else {
	    sarea->nbox = nr - i;
	    for ( ; i < nr ; i++, b++) {
	       b->x1 = pbox[i].x1 - imesa->drawX;
	       b->y1 = pbox[i].y1 - imesa->drawY;
	       b->x2 = pbox[i].x2 - imesa->drawX;
	       b->y2 = pbox[i].y2 - imesa->drawY;
	    }
	 }
	 
	 /* Finished with the buffer?
	  */
	 if (nr == nbox) 
	    vertex.discard = 1;

	 drmCommandWrite(imesa->driFd, DRM_I810_VERTEX,
                         &vertex, sizeof(drmI810Vertex));
	 age_imesa(imesa, imesa->sarea->last_enqueue);
      }
   }

   /* Reset imesa vars:
    */
   imesa->vertex_buffer = 0;
   imesa->vertex_addr = 0;
   imesa->vertex_low = 0;
   imesa->vertex_high = 0;
   imesa->vertex_last_prim = 0;
   imesa->dirty = 0;
   imesa->upload_cliprects = GL_FALSE;
}

void i810FlushPrimsGetBuffer( i810ContextPtr imesa )
{
   LOCK_HARDWARE(imesa);

   if (imesa->vertex_buffer) 
      i810FlushPrimsLocked( imesa );      

   imesa->vertex_buffer = i810_get_buffer_ioctl( imesa );
   imesa->vertex_high = imesa->vertex_buffer->total;
   imesa->vertex_addr = (char *)imesa->vertex_buffer->address;
   imesa->vertex_low = 4;	/* leave room for instruction header */
   imesa->vertex_last_prim = imesa->vertex_low;
   UNLOCK_HARDWARE(imesa);
}


void i810FlushPrims( i810ContextPtr imesa ) 
{
   if (imesa->vertex_buffer) {
      LOCK_HARDWARE( imesa );
      i810FlushPrimsLocked( imesa );
      UNLOCK_HARDWARE( imesa );
   }
}



int i810_check_copy(int fd)
{
   return(drmCommandNone(fd, DRM_I810_DOCOPY));
}

static void i810Flush( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   I810_FIREVERTICES( imesa );
}

static void i810Finish( GLcontext *ctx  ) 
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   i810DmaFinish( imesa );
}

void i810InitIoctlFuncs( struct dd_function_table *functions )
{
   functions->Flush = i810Flush;
   functions->Clear = i810Clear;
   functions->Finish = i810Finish;
}
