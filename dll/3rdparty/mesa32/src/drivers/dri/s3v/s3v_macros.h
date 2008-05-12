/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#ifndef _S3V_MACROS_H_
#define _S3V_MACROS_H_

/**************/
/* DRI macros */
/**************/

#define GENERIC_DEBUG 0
#define FLOW_DEBUG    0
#define DMABUFS_DEBUG 0

/* Note: The argument to DEBUG*() _must_ be enclosed in parenthesis */

#if (GENERIC_DEBUG || FLOW_DEBUG || DMABUFS_DEBUG)
#include <stdio.h>
#endif

#if GENERIC_DEBUG
#define DEBUG(str) printf str
#else
#define DEBUG(str)
#endif

#if FLOW_DEBUG
#define DEBUG_WHERE(str) printf str
#else
#define DEBUG_WHERE(str)
#endif

#if DMABUFS_DEBUG
#define DEBUG_BUFS(str) printf str
#else
#define DEBUG_BUFS(str)
#endif


#if 0
#define S3V_DMA_SEND_FLAGS    DRM_DMA_PRIORITY
#define S3V_DMA_SEND_FLAGS    DRM_DMA_BLOCK
#else
#define S3V_DMA_SEND_FLAGS    0
#endif

#if 0
#define S3V_DMA_GET_FLAGS     \
    (DRM_DMA_SMALLER_OK | DRM_DMA_LARGER_OK | DRM_DMA_WAIT)
#else
#define S3V_DMA_GET_FLAGS     DRM_DMA_WAIT
#endif


#define DMAOUT_CHECK(reg,len) \
do { \
	DEBUG(("DMAOUT_CHECK: reg = 0x%x\n", S3V_##reg##_REG)); \
	DEBUG_BUFS(("DMAOUT_CHECK (was): ")); \
	DEBUG_BUFS(("vmesa->bufCount=%i of vmesa->bufSize=%i\n", \
		vmesa->bufCount, vmesa->bufSize)); \
	/* FIXME: > or >= */ \
	if (vmesa->bufCount+(len+1) >= vmesa->bufSize) \
		DMAFLUSH(); \
\
	vmesa->bufCount += (len+1); \
	DEBUG_BUFS(("DMAOUT_CHECK (is): vmesa->bufCount=%i len=%i, reg=%x\n", \
		vmesa->bufCount, len, S3V_##reg##_REG)); \
	DMAOUT(	((len & 0xffff) | ((S3V_##reg##_REG & 0xfffc) << 14)) );  \
} while (0)

#define DMAOUT(val) \
do { \
	*(vmesa->buf++)=val; \
	DEBUG_BUFS(("DMAOUT: val=0x%x\n", (unsigned int)val)); \
} while(0)

#define DMAFINISH()	\
do { \
	/* NOTE: it does nothing - it just prints some summary infos */ \
	DEBUG(("DMAFINISH: vmesa->bufCount=%i\n", vmesa->bufCount)); \
	DEBUG(("buf: index=%i; addr=%p\n", vmesa->bufIndex[vmesa->_bufNum], \
	vmesa->s3vScreen->bufs->list[vmesa->bufIndex[vmesa->_bufNum]].address)); \
} while(0)

#define DMAFLUSH() \
do { \
	if (vmesa->bufCount) { \
		SEND_DMA(vmesa->driFd, vmesa->hHWContext, 1, \
			&vmesa->bufIndex[vmesa->_bufNum], &vmesa->bufCount); \
/*
		GET_DMA(vmesa->driFd, vmesa->hHWContext, 1, \
			&vmesa->bufIndex, &vmesa->bufSize); \
*/ \
		vmesa->_bufNum = !(vmesa->_bufNum); \
		vmesa->buf = vmesa->_buf[vmesa->_bufNum]; \
/* 
		vmesa->buf = \
			vmesa->s3vScreen->bufs->list[vmesa->bufIndex].address; \
*/ \
		vmesa->bufCount = 0; \
	} \
} while (0)

#define CMDCHANGE() \
do { \
	DMAOUT_CHECK(3DTRI_CMDSET, 1); /* FIXME: TRI/LINE */ \
		DMAOUT(vmesa->CMD); \
	DMAFINISH(); \
} while (0)

#ifdef DONT_SEND_DMA
#define GET_DMA(fd, hHWCtx, n, idx, size)
#define SEND_DMA(fd, hHWCtx,n, idx, cnt)
#else
#define GET_DMA(fd, hHWCtx, n, idx, size) \
do { \
	drmDMAReq dma; \
	int retcode, i; \
\
	DEBUG(("GET_DMA: ")); \
	DEBUG(("req_count=%i; req_list[#0]=%i; req_size[#0]=%i\n", \
		n, (idx)[n-1], (size)[n-1])); \
\
	dma.context       = (hHWCtx); \
	dma.send_count    = 0; \
	dma.send_list     = NULL; \
	dma.send_sizes    = NULL; \
	dma.flags         = S3V_DMA_GET_FLAGS; \
	dma.request_count = (n); \
	dma.request_size  = S3V_DMA_BUF_SZ; \
	dma.request_list  = (idx); \
	dma.request_sizes = (size); \
\
	do { \
		if ((retcode = drmDMA((fd), &dma))) { \
		DEBUG_BUFS(("drmDMA (get) returned %d\n", retcode)); \
	} \
} while (!(dma).granted_count); \
\
	for (i = 0; i < (n); i++) { \
		DEBUG(("Got buffer %i (index #%i)\n", (idx)[i], i)); \
		DEBUG(("of %i bytes (%i words) size\n", \
			(size)[i], (size)[i] >>2)); \
		/* Convert from bytes to words */ \
		(size)[i] >>= 2; \
	} \
} while (0)

#define SEND_DMA(fd, hHWCtx, n, idx, cnt) \
do { \
	drmDMAReq dma; \
	int retcode, i; \
\
	DEBUG(("SEND_DMA: ")); \
	DEBUG(("send_count=%i; send_list[#0]=%i; send_sizes[#0]=%i\n", \
		n, (idx)[n-1], (cnt)[n-1])); \
\
	for (i = 0; i < (n); i++) { \
		/* Convert from words to bytes */ \
		(cnt)[i] <<= 2; \
	} \
\
	dma.context       = (hHWCtx); \
	dma.send_count    = (n); \
	dma.send_list     = (idx); \
	dma.send_sizes    = (cnt); \
	dma.flags         = S3V_DMA_SEND_FLAGS; \
	dma.request_count = 0; \
	dma.request_size  = 0; \
	dma.request_list  = NULL; \
	dma.request_sizes = NULL; \
\
	if ((retcode = drmDMA((fd), &dma))) { \
		DEBUG_BUFS(("drmDMA (send) returned %d\n", retcode)); \
	} \
\
	for (i = 0; i < (n); i++) { \
		DEBUG(("Sent buffer %i (index #%i)\n", (idx)[i], i)); \
		DEBUG(("of %i bytes (%i words) size\n", \
			(cnt)[i], (cnt)[i] >>2)); \
		(cnt)[i] = 0; \
	} \
} while (0)
#endif /* DONT_SEND_DMA */

#define GET_FIRST_DMA(fd, hHWCtx, n, idx, size, buf, cnt, vPriv) \
do { \
	int i; \
	DEBUG_BUFS(("GET_FIRST_DMA\n")); \
	DEBUG_BUFS(("n=%i idx=%i size=%i\n", n, *idx, *size)); \
	DEBUG_BUFS(("going to GET_DMA\n")); \
	GET_DMA(fd, hHWCtx, n, idx, size); \
	DEBUG_BUFS(("coming from GET_DMA\n")); \
	DEBUG_BUFS(("n=%i idx=%i size=%i\n", n, (idx)[0], (size)[0])); \
	for (i = 0; i < (n); i++) { \
		DEBUG_BUFS(("buf #%i @%p\n", \
			i, (vPriv)->bufs->list[(idx)[i]].address)); \
		(buf)[i] = (vPriv)->bufs->list[(idx)[i]].address; \
		(cnt)[i] = 0; \
	} \
	DEBUG(("GOING HOME\n")); \
} while (0)

/**************************/
/* generic, global macros */
/**************************/

#define CALC_LOG2(l2,s) \
do { \
	int __s = s; \
	l2 = 0; \
	while (__s > 1) { ++l2; __s >>= 1; } \
} while (0)

#define PrimType_Null               0x00000000
#define PrimType_Points             0x10000000
#define PrimType_Lines              0x20000000
#define PrimType_LineLoop           0x30000000
#define PrimType_LineStrip          0x40000000
#define PrimType_Triangles          0x50000000
#define PrimType_TriangleStrip      0x60000000
#define PrimType_TriangleFan        0x70000000
#define PrimType_Quads              0x80000000
#define PrimType_QuadStrip          0x90000000
#define PrimType_Polygon            0xa0000000
#define PrimType_Mask               0xf0000000

#endif /* _S3V_MACROS_H_ */
