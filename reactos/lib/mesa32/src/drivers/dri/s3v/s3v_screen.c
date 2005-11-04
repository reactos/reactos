/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "s3v_context.h"
#include "s3v_vb.h"
#include "s3v_dri.h" 

s3vScreenPtr s3vCreateScreen( __DRIscreenPrivate *sPriv )
{
   s3vScreenPtr s3vScreen;
   S3VDRIPtr vDRIPriv = (S3VDRIPtr)sPriv->pDevPriv;

/*   int i; */

   DEBUG(("s3vCreateScreen\n"));
   DEBUG(("sPriv->pDevPriv at %p\n", sPriv->pDevPriv));
   DEBUG(("size = %i\n", sizeof(*vDRIPriv)));

   if (sPriv->devPrivSize != sizeof(S3VDRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(S3VDRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   s3vScreen = (s3vScreenPtr) CALLOC( sizeof(*s3vScreen) );
   if ( !s3vScreen ) return NULL;

   s3vScreen->regionCount  = 4;	/* Magic number.  Can we fix this? */
    
   s3vScreen->regions = _mesa_malloc(s3vScreen->regionCount * 
							sizeof(s3vRegion));
   DEBUG(("sPriv->fd = %i\nvDRIPriv->dmaBufHandle = %x\n",
      sPriv->fd, vDRIPriv->dmaBufHandle));

   DEBUG(("vDRIPriv->dmaBufSize=%i\nvDRIPriv->dmaBuf=%p\n",
      vDRIPriv->dmaBufSize, vDRIPriv->dmaBuf));


   /* Get the list of dma buffers */
   s3vScreen->bufs = drmMapBufs(sPriv->fd);

   if (!s3vScreen->bufs) {
      DEBUG(("Helter/skelter with drmMapBufs\n"));
      return GL_FALSE; 
   }

   s3vScreen->textureSize		    = vDRIPriv->texSize;
   s3vScreen->logTextureGranularity = vDRIPriv->logTextureGranularity;
   s3vScreen->cpp 					= vDRIPriv->cpp;
   s3vScreen->frontOffset			= vDRIPriv->frontOffset;
   s3vScreen->frontPitch			= vDRIPriv->frontPitch;
   s3vScreen->backOffset			= vDRIPriv->backOffset;
   s3vScreen->backPitch				= vDRIPriv->frontPitch; /* FIXME: check */
   s3vScreen->depthOffset			= vDRIPriv->depthOffset;
   s3vScreen->depthPitch			= vDRIPriv->frontPitch;
   s3vScreen->texOffset				= vDRIPriv->texOffset;

   s3vScreen->driScreen = sPriv;

   DEBUG(("vDRIPriv->width =%i; vDRIPriv->deviceID =%x\n", vDRIPriv->width,
		  vDRIPriv->deviceID));
   DEBUG(("vDRIPriv->mem =%i\n", vDRIPriv->mem));
   DEBUG(("vDRIPriv->fbOffset =%i\n", vDRIPriv->fbOffset));
   DEBUG((" ps3vDRI->fbStride =%i\n", vDRIPriv->fbStride));
   DEBUG(("s3vScreen->cpp = %i\n", s3vScreen->cpp));
   DEBUG(("s3vScreen->backOffset = %x\n", s3vScreen->backOffset));
   DEBUG(("s3vScreen->depthOffset = %x\n", s3vScreen->depthOffset));
   DEBUG(("s3vScreen->texOffset = %x\n", s3vScreen->texOffset));
   DEBUG(("I will return from s3vCreateScreen now\n"));
   
   DEBUG(("s3vScreen->bufs = 0x%x\n", s3vScreen->bufs));
   return s3vScreen;
}

/* Destroy the device specific screen private data struct.
 */
void s3vDestroyScreen( __DRIscreenPrivate *sPriv )
{
    s3vScreenPtr s3vScreen = (s3vScreenPtr)sPriv->private;

    DEBUG(("s3vDestroyScreen\n"));

    /* First, unmap the dma buffers */
/*
    drmUnmapBufs( s3vScreen->bufs );
*/
    /* Next, unmap all the regions */
/*    while (s3vScreen->regionCount > 0) { 

	(void)drmUnmap(s3vScreen->regions[s3vScreen->regionCount].map,
		       s3vScreen->regions[s3vScreen->regionCount].size);
	s3vScreen->regionCount--;

    }
    FREE(s3vScreen->regions); */
	if (s3vScreen)
	    FREE(s3vScreen);
}
