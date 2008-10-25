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



#ifndef __NOUVEAU_FIFO_H__
#define __NOUVEAU_FIFO_H__

#include "nouveau_context.h"
#include "nouveau_ctrlreg.h"
#include "nouveau_state_cache.h"

//#define NOUVEAU_RING_TRACE
//#define NOUVEAU_RING_DEBUG
//#define NOUVEAU_STATE_CACHE_DISABLE

#ifndef NOUVEAU_RING_TRACE
#define NOUVEAU_RING_TRACE 0
#else
#undef NOUVEAU_RING_TRACE
#define NOUVEAU_RING_TRACE 1
#endif

#define NV_READ(reg) *(volatile u_int32_t *)(nmesa->mmio + (reg))

#define NV_FIFO_READ(reg) *(volatile u_int32_t *)(nmesa->fifo.mmio + (reg/4))
#define NV_FIFO_WRITE(reg,value) *(volatile u_int32_t *)(nmesa->fifo.mmio + (reg/4)) = value;
#define NV_FIFO_READ_GET() ((NV_FIFO_READ(NV03_FIFO_REGS_DMAGET) - nmesa->fifo.put_base) >> 2)
#define NV_FIFO_WRITE_PUT(val) do { \
	if (NOUVEAU_RING_TRACE) {\
		printf("FIRE_RING : 0x%08x\n", nmesa->fifo.current << 2); \
		fflush(stdout); \
		sleep(1); \
	} \
	NV_FIFO_WRITE(NV03_FIFO_REGS_DMAPUT, ((val)<<2) + nmesa->fifo.put_base); \
} while(0)

/* 
 * Ring/fifo interface
 *
 * - Begin a ring section with BEGIN_RING_SIZE (if you know the full size in advance)
 * - Output stuff to the ring with either OUT_RINGp (outputs a raw mem chunk), OUT_RING (1 uint32_t) or OUT_RINGf (1 float)
 * - RING_AVAILABLE returns the available fifo (in uint32_ts)
 * - RING_AHEAD returns how much ahead of the last submission point we are
 * - FIRE_RING fires whatever we have that wasn't fired before
 * - WAIT_RING waits for size (in uint32_ts) to be available in the fifo
 */

/* Enable for ring debugging.  Prints out writes to the ring buffer
 * but does not actually write to it.
 */
#ifdef NOUVEAU_RING_DEBUG

extern int nouveau_fifo_remaining;

#define OUT_RINGp(ptr,sz) do {                                                  \
uint32_t* p=(uint32_t*)(ptr);							\
int i; printf("OUT_RINGp: (size 0x%x dwords)\n",sz); for(i=0;i<sz;i++) printf(" 0x%08x   %f\n", *(p+i), *((float*)(p+i))); 	\
nouveau_fifo_remaining-=sz;							\
}while(0)

#define OUT_RING(n) do {                                                        \
    printf("OUT_RINGn: 0x%08x (%s)\n", n, __func__);                            \
    nouveau_fifo_remaining--;							\
}while(0)

#define OUT_RINGf(n) do {                                                       \
    printf("OUT_RINGf: %.04f (%s)\n", n, __func__);                             \
    nouveau_fifo_remaining--;							\
}while(0)

#define BEGIN_RING_SIZE(subchannel,tag,size) do {					\
	if (nouveau_fifo_remaining!=0)							\
		printf("RING ERROR : remaining %d\n",nouveau_fifo_remaining);		\
	nouveau_state_cache_flush(nmesa);						\
	if (nmesa->fifo.free <= (size))							\
		WAIT_RING(nmesa,(size));						\
	OUT_RING( ((size)<<18) | ((subchannel) << 13) | (tag));				\
	nmesa->fifo.free -= ((size) + 1);                                               \
	nouveau_fifo_remaining=size;							\
}while(0)

#else

#define OUT_RINGp(ptr,sz) do{							\
	if (NOUVEAU_RING_TRACE) { \
		uint32_t* p=(uint32_t*)(ptr);							\
		int i; printf("OUT_RINGp: (size 0x%x dwords) (%s)\n",sz, __func__); for(i=0;i<sz;i++) printf(" [0x%08x] 0x%08x   %f\n", (nmesa->fifo.current+i) << 2, *(p+i), *((float*)(p+i))); 	\
	} \
	memcpy(nmesa->fifo.buffer+nmesa->fifo.current,ptr,(sz)*4);		\
	nmesa->fifo.current+=(sz);						\
}while(0)

#define OUT_RING(n) do {							\
if (NOUVEAU_RING_TRACE) \
    printf("OUT_RINGn: [0x%08x] 0x%08x (%s)\n", nmesa->fifo.current << 2, n, __func__);        \
nmesa->fifo.buffer[nmesa->fifo.current++]=(n);					\
}while(0)

#define OUT_RINGf(n) do {							\
if (NOUVEAU_RING_TRACE) \
    printf("OUT_RINGf: [0x%08x] %.04f (%s)\n", nmesa->fifo.current << 2, n, __func__);        \
*((float*)(nmesa->fifo.buffer+nmesa->fifo.current++))=(n);			\
}while(0)

#define BEGIN_RING_SIZE(subchannel,tag,size) do {					\
	nouveau_state_cache_flush(nmesa);						\
	if (nmesa->fifo.free <= (size))							\
		WAIT_RING(nmesa,(size));						\
	OUT_RING( ((size)<<18) | ((subchannel) << 13) | (tag));				\
	nmesa->fifo.free -= ((size) + 1);                                               \
}while(0)

#endif

extern void WAIT_RING(nouveauContextPtr nmesa,u_int32_t size);
extern void nouveau_state_cache_flush(nouveauContextPtr nmesa);
extern void nouveau_state_cache_init(nouveauContextPtr nmesa);

#ifdef NOUVEAU_STATE_CACHE_DISABLE
#define BEGIN_RING_CACHE(subc,tag,size) BEGIN_RING_SIZE((subc), (tag), (size))
#define OUT_RING_CACHE(n) OUT_RING((n))
#define OUT_RING_CACHEf(n) OUT_RINGf((n))
#define OUT_RING_CACHEp(ptr, sz) OUT_RINGp((ptr), (sz))
#else
#define BEGIN_RING_CACHE(subchannel,tag,size) do {					\
	nmesa->state_cache.dirty=1;	 						\
	nmesa->state_cache.current_pos=((tag)/4);					\
}while(0)

#define OUT_RING_CACHE(n) do {									\
	if (nmesa->state_cache.atoms[nmesa->state_cache.current_pos].value!=(n))	{	\
		nmesa->state_cache.atoms[nmesa->state_cache.current_pos].dirty=1; 		\
		nmesa->state_cache.hdirty[nmesa->state_cache.current_pos/NOUVEAU_STATE_CACHE_HIER_SIZE]=1; 		\
		nmesa->state_cache.atoms[nmesa->state_cache.current_pos].value=(n);		\
	}											\
	nmesa->state_cache.current_pos++;							\
}while(0)

#define OUT_RING_CACHEf(n) do {									\
	if ((*(float*)(&nmesa->state_cache.atoms[nmesa->state_cache.current_pos].value))!=(n)){	\
		nmesa->state_cache.atoms[nmesa->state_cache.current_pos].dirty=1;	 	\
		nmesa->state_cache.hdirty[nmesa->state_cache.current_pos/NOUVEAU_STATE_CACHE_HIER_SIZE]=1; 		\
		(*(float*)(&nmesa->state_cache.atoms[nmesa->state_cache.current_pos].value))=(n);\
	}											\
	nmesa->state_cache.current_pos++;							\
}while(0)

#define OUT_RING_CACHEp(ptr,sz) do {							\
uint32_t* p=(uint32_t*)(ptr);								\
int i; for(i=0;i<sz;i++) OUT_RING_CACHE(*(p+i)); 					\
}while(0)
#endif

#define RING_AVAILABLE() (nmesa->fifo.free-1)

#define RING_AHEAD() ((nmesa->fifo.put<=nmesa->fifo.current)?(nmesa->fifo.current-nmesa->fifo.put):nmesa->fifo.max-nmesa->fifo.put+nmesa->fifo.current)

#define FIRE_RING() do {                             \
	if (nmesa->fifo.current!=nmesa->fifo.put) {  \
		nmesa->fifo.put=nmesa->fifo.current; \
		NV_FIFO_WRITE_PUT(nmesa->fifo.put);  \
	}                                            \
}while(0)

extern void nouveauWaitForIdle(nouveauContextPtr nmesa);
extern void nouveauWaitForIdleLocked(nouveauContextPtr nmesa);
extern GLboolean nouveauFifoInit(nouveauContextPtr nmesa);

#endif /* __NOUVEAU_FIFO_H__ */


