#ifndef __NOUVEAU_OBJECT_H__
#define __NOUVEAU_OBJECT_H__

#include "nouveau_context.h"

#define ALLOW_MULTI_SUBCHANNEL

void nouveauObjectInit(nouveauContextPtr nmesa);

enum DMAObjects {
	Nv3D                    = 0x80000019,
	NvCtxSurf2D		= 0x80000020,
	NvImageBlit		= 0x80000021,
	NvMemFormat		= 0x80000022,
	NvCtxSurf3D		= 0x80000023,
	NvDmaFB			= 0xD0FB0001,
	NvDmaAGP		= 0xD0AA0001,
	NvSyncNotify		= 0xD0000001,
	NvQueryNotify		= 0xD0000002
};

enum DMASubchannel {
	NvSubCtxSurf2D	= 0,
	NvSubImageBlit	= 1,
	NvSubMemFormat	= 2,
	NvSubCtxSurf3D	= 3,
	NvSub3D		= 7,
};

extern void nouveauObjectOnSubchannel(nouveauContextPtr nmesa, int subchannel, int handle);

extern GLboolean nouveauCreateContextObject(nouveauContextPtr nmesa,
      					    uint32_t handle, int class);
extern GLboolean nouveauCreateDmaObject(nouveauContextPtr nmesa,
      					uint32_t handle,
					int      class,
					uint32_t offset,
					uint32_t size,
					int      target,
					int      access);
extern GLboolean nouveauCreateDmaObjectFromMem(nouveauContextPtr nmesa,
					       uint32_t     handle,
					       int          class,
					       nouveau_mem *mem,
					       int          access);

#endif
