
#include "nouveau_fifo.h"
#include "nouveau_object.h"
#include "nouveau_reg.h"


GLboolean nouveauCreateContextObject(nouveauContextPtr nmesa,
				     uint32_t handle, int class)
{
	drm_nouveau_object_init_t cto;
	int ret;

	cto.channel = nmesa->fifo.channel;
	cto.handle  = handle;
	cto.class   = class;
	ret = drmCommandWrite(nmesa->driFd, DRM_NOUVEAU_OBJECT_INIT, &cto, sizeof(cto));

	return ret == 0;
}

GLboolean nouveauCreateDmaObject(nouveauContextPtr nmesa,
      				 uint32_t handle,
				 int      class,
				 uint32_t offset,
				 uint32_t size,
				 int	  target,
				 int	  access)
{
	drm_nouveau_dma_object_init_t dma;
	int ret;

	dma.channel = nmesa->fifo.channel;
	dma.class   = class;
	dma.handle  = handle;
	dma.target  = target;
	dma.access  = access;
	dma.offset  = offset;
	dma.size    = size;
	ret = drmCommandWriteRead(nmesa->driFd, DRM_NOUVEAU_DMA_OBJECT_INIT,
				  &dma, sizeof(dma));
	return ret == 0;
}

GLboolean nouveauCreateDmaObjectFromMem(nouveauContextPtr nmesa,
					uint32_t handle, int class,
					nouveau_mem *mem,
					int access)
{
	uint32_t offset = mem->offset;
	int target = mem->type & (NOUVEAU_MEM_FB | NOUVEAU_MEM_AGP);

	if (!target)
		return GL_FALSE;

	if (target & NOUVEAU_MEM_FB)
		offset -= nmesa->vram_phys;
	else if (target & NOUVEAU_MEM_AGP)
		offset -= nmesa->agp_phys;

	return nouveauCreateDmaObject(nmesa, handle, class,
				      offset, mem->size,
				      target, access);
}

void nouveauObjectOnSubchannel(nouveauContextPtr nmesa, int subchannel, int handle)
{
	BEGIN_RING_SIZE(subchannel, 0, 1);
	OUT_RING(handle);
}

void nouveauObjectInit(nouveauContextPtr nmesa)
{
#ifdef NOUVEAU_RING_DEBUG
	return;
#endif

/* We need to know vram size.. and AGP size (and even if the card is AGP..) */
	nouveauCreateDmaObject( nmesa, NvDmaFB, NV_DMA_IN_MEMORY,
				0, nmesa->vram_size,
				NOUVEAU_MEM_FB,
				NOUVEAU_MEM_ACCESS_RW);
	nouveauCreateDmaObject( nmesa, NvDmaAGP, NV_DMA_IN_MEMORY,
	      			0, nmesa->agp_size,
				NOUVEAU_MEM_AGP,
				NOUVEAU_MEM_ACCESS_RW);

	nouveauCreateContextObject(nmesa, Nv3D, nmesa->screen->card->class_3d);
	if (nmesa->screen->card->type>=NV_10) {
		nouveauCreateContextObject(nmesa, NvCtxSurf2D, NV10_CONTEXT_SURFACES_2D);
		nouveauCreateContextObject(nmesa, NvImageBlit, NV10_IMAGE_BLIT);
	} else {
		nouveauCreateContextObject(nmesa, NvCtxSurf2D, NV04_CONTEXT_SURFACES_2D);
		nouveauCreateContextObject(nmesa, NvCtxSurf3D, NV04_CONTEXT_SURFACES_3D);
		nouveauCreateContextObject(nmesa, NvImageBlit, NV_IMAGE_BLIT);
	}
	nouveauCreateContextObject(nmesa, NvMemFormat, NV_MEMORY_TO_MEMORY_FORMAT);

#ifdef ALLOW_MULTI_SUBCHANNEL
	nouveauObjectOnSubchannel(nmesa, NvSubCtxSurf2D, NvCtxSurf2D);
	BEGIN_RING_SIZE(NvSubCtxSurf2D, NV10_CONTEXT_SURFACES_2D_SET_DMA_IN_MEMORY0, 2);
	OUT_RING(NvDmaFB);
	OUT_RING(NvDmaFB);

	nouveauObjectOnSubchannel(nmesa, NvSubImageBlit, NvImageBlit);
	BEGIN_RING_SIZE(NvSubImageBlit, NV10_IMAGE_BLIT_SET_CONTEXT_SURFACES_2D, 1);
	OUT_RING(NvCtxSurf2D);
	BEGIN_RING_SIZE(NvSubImageBlit, NV10_IMAGE_BLIT_SET_OPERATION, 1);
	OUT_RING(3); /* SRCCOPY */

	nouveauObjectOnSubchannel(nmesa, NvSubMemFormat, NvMemFormat);
#endif

	nouveauObjectOnSubchannel(nmesa, NvSub3D, Nv3D);
}



