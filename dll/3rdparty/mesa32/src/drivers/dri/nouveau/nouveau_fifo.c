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


#include "vblank.h"
#include <errno.h>
#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "swrast/swrast.h"
#include "nouveau_context.h"
#include "nouveau_msg.h"
#include "nouveau_fifo.h"
#include "nouveau_lock.h"
#include "nouveau_object.h"
#include "nouveau_sync.h"

#ifdef NOUVEAU_RING_DEBUG
int nouveau_fifo_remaining=0;
#endif


#define RING_SKIPS 8

void WAIT_RING(nouveauContextPtr nmesa,u_int32_t size)
{
#ifdef NOUVEAU_RING_DEBUG
	return;
#endif
	u_int32_t fifo_get;
	while(nmesa->fifo.free < size+1) {
		fifo_get = NV_FIFO_READ_GET();

		if(nmesa->fifo.put >= fifo_get) {
			nmesa->fifo.free = nmesa->fifo.max - nmesa->fifo.current;
			if(nmesa->fifo.free < size+1) {
				OUT_RING(NV03_FIFO_CMD_JUMP | nmesa->fifo.put_base);
				if(fifo_get <= RING_SKIPS) {
					if(nmesa->fifo.put <= RING_SKIPS) /* corner case - will be idle */
						NV_FIFO_WRITE_PUT(RING_SKIPS + 1);
					do { fifo_get = NV_FIFO_READ_GET(); }
					while(fifo_get <= RING_SKIPS);
				}
				NV_FIFO_WRITE_PUT(RING_SKIPS);
				nmesa->fifo.current = nmesa->fifo.put = RING_SKIPS;
				nmesa->fifo.free = fifo_get - (RING_SKIPS + 1);
			}
		} else 
			nmesa->fifo.free = fifo_get - nmesa->fifo.current - 1;
	}
}

/* 
 * Wait for the channel to be idle 
 */
void nouveauWaitForIdleLocked(nouveauContextPtr nmesa)
{
	/* Wait for FIFO idle */
	FIRE_RING();
	while(RING_AHEAD()>0);

	/* Wait on notifier to indicate all commands in the channel have
	 * been completed.
	 */
	nouveau_notifier_wait_nop(nmesa->glCtx, nmesa->syncNotifier, NvSub3D);
}

void nouveauWaitForIdle(nouveauContextPtr nmesa)
{
	LOCK_HARDWARE(nmesa);
	nouveauWaitForIdleLocked(nmesa);
	UNLOCK_HARDWARE(nmesa);
}

// here we call the fifo initialization ioctl and fill in stuff accordingly
GLboolean nouveauFifoInit(nouveauContextPtr nmesa)
{
	drm_nouveau_fifo_alloc_t fifo_init;
	int i;

#ifdef NOUVEAU_RING_DEBUG
	return GL_TRUE;
#endif

	int ret;
	ret=drmCommandWriteRead(nmesa->driFd, DRM_NOUVEAU_FIFO_ALLOC, &fifo_init, sizeof(fifo_init));
	if (ret) {
		FATAL("Fifo initialization ioctl failed (returned %d)\n",ret);
		return GL_FALSE;
	}

	ret = drmMap(nmesa->driFd, fifo_init.cmdbuf, fifo_init.cmdbuf_size, &nmesa->fifo.buffer);
	if (ret) {
		FATAL("Unable to map the fifo (returned %d)\n",ret);
		return GL_FALSE;
	}
	ret = drmMap(nmesa->driFd, fifo_init.ctrl, fifo_init.ctrl_size, &nmesa->fifo.mmio);
	if (ret) {
		FATAL("Unable to map the control regs (returned %d)\n",ret);
		return GL_FALSE;
	}

	/* Setup our initial FIFO tracking params */
	nmesa->fifo.channel  = fifo_init.channel;
	nmesa->fifo.put_base = fifo_init.put_base;
	nmesa->fifo.current  = 0;
	nmesa->fifo.put      = 0;
	nmesa->fifo.max      = (fifo_init.cmdbuf_size >> 2) - 1;
	nmesa->fifo.free     = nmesa->fifo.max - nmesa->fifo.current;

	for (i=0; i<RING_SKIPS; i++)
	   OUT_RING(0);
	nmesa->fifo.free -= RING_SKIPS;

	MESSAGE("Fifo init ok. Using context %d\n", fifo_init.channel);
	return GL_TRUE;
}


