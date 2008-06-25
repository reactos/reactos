/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef SAVAGE_IOCTL_H
#define SAVAGE_IOCTL_H

#include "savagecontext.h"

void savageFlushVertices( savageContextPtr mmesa ); 

unsigned int savageEmitEventLocked( savageContextPtr imesa, unsigned int flags );
unsigned int savageEmitEvent( savageContextPtr imesa, unsigned int flags );
void savageWaitEvent( savageContextPtr imesa, unsigned int event);

void savageFlushCmdBufLocked( savageContextPtr imesa, GLboolean discard );
void savageFlushCmdBuf( savageContextPtr imesa, GLboolean discard );

void savageDDInitIoctlFuncs( GLcontext *ctx );

void savageSwapBuffers( __DRIdrawablePrivate *dPriv );

#define WAIT_IDLE_EMPTY(imesa) do { \
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG) \
        fprintf (stderr, "WAIT_IDLE_EMPTY in %s\n", __FUNCTION__); \
    savageWaitEvent(imesa, \
		    savageEmitEvent(imesa, SAVAGE_WAIT_2D|SAVAGE_WAIT_3D)); \
} while (0)

#define WAIT_IDLE_EMPTY_LOCKED(imesa) do { \
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG) \
        fprintf (stderr, "WAIT_IDLE_EMPTY_LOCKED in %s\n", __FUNCTION__); \
    savageWaitEvent(imesa, savageEmitEventLocked( \
			imesa, SAVAGE_WAIT_2D|SAVAGE_WAIT_3D)); \
} while (0)

#define FLUSH_BATCH(imesa) do { \
    if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG) \
        fprintf (stderr, "FLUSH_BATCH in %s\n", __FUNCTION__); \
    savageFlushVertices(imesa); \
    savageFlushCmdBuf(imesa, GL_FALSE); \
} while (0)

extern void savageGetDMABuffer( savageContextPtr imesa );

static __inline
void savageReleaseIndexedVerts( savageContextPtr imesa )
{
    imesa->firstElt = -1;
}

static __inline
GLboolean savageHaveIndexedVerts( savageContextPtr imesa )
{
    return (imesa->firstElt != -1);
}

static __inline
u_int32_t *savageAllocVtxBuf( savageContextPtr imesa, GLuint words )
{
   struct savage_vtxbuf_t *buffer = imesa->vtxBuf;
   u_int32_t *head;

   if (buffer == &imesa->dmaVtxBuf) {
       if (!buffer->total) {
	   LOCK_HARDWARE(imesa);
	   savageGetDMABuffer(imesa);
	   UNLOCK_HARDWARE(imesa);
       } else if (buffer->used + words > buffer->total) {
	   if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
	       fprintf (stderr, "... flushing DMA buffer in %s\n",
			__FUNCTION__);
	   savageReleaseIndexedVerts(imesa);
	   savageFlushVertices(imesa);
	   LOCK_HARDWARE(imesa);
	   savageFlushCmdBufLocked(imesa, GL_TRUE); /* discard DMA buffer */
	   savageGetDMABuffer(imesa);
	   UNLOCK_HARDWARE(imesa);
       }
   } else if (buffer->used + words > buffer->total) {
       if (SAVAGE_DEBUG & DEBUG_VERBOSE_MSG)
	   fprintf (stderr, "... flushing client vertex buffer in %s\n",
		    __FUNCTION__);
       savageReleaseIndexedVerts(imesa);
       savageFlushVertices(imesa);
       LOCK_HARDWARE(imesa);
       savageFlushCmdBufLocked(imesa, GL_FALSE); /* free clientVtxBuf */
       UNLOCK_HARDWARE(imesa);
   }

   head = &buffer->buf[buffer->used];

   buffer->used += words;
   return head;
}

static __inline
u_int32_t *savageAllocIndexedVerts( savageContextPtr imesa, GLuint n )
{
    u_int32_t *ret;
    savageFlushVertices(imesa);
    ret = savageAllocVtxBuf(imesa, n*imesa->HwVertexSize);
    imesa->firstElt = imesa->vtxBuf->flushed / imesa->HwVertexSize;
    imesa->vtxBuf->flushed = imesa->vtxBuf->used;
    return ret;
}

/* Flush Elts:
 * - Complete the drawing command with the correct number of indices.
 * - Actually allocate entries for the indices in the command buffer.
 *   (This allocation must succeed without wrapping the cmd buffer!)
 */
static __inline
void savageFlushElts( savageContextPtr imesa )
{
    if (imesa->elts.cmd) {
	GLuint qwords = (imesa->elts.n + 3) >> 2;
	assert(imesa->cmdBuf.write - imesa->cmdBuf.base + qwords
	       <= imesa->cmdBuf.size);
	imesa->cmdBuf.write += qwords;

	imesa->elts.cmd->idx.count = imesa->elts.n;
	imesa->elts.cmd = NULL;
    }
}

/* Allocate a command buffer entry with <bytes> bytes of arguments:
 * - implies savageFlushElts
 */
static __inline
drm_savage_cmd_header_t *savageAllocCmdBuf( savageContextPtr imesa, GLuint bytes )
{
    drm_savage_cmd_header_t *ret;
    GLuint qwords = ((bytes + 7) >> 3) + 1; /* round up */
    assert (qwords < imesa->cmdBuf.size);

    savageFlushElts(imesa);

    if (imesa->cmdBuf.write - imesa->cmdBuf.base + qwords > imesa->cmdBuf.size)
	savageFlushCmdBuf(imesa, GL_FALSE);

    ret = (drm_savage_cmd_header_t *)imesa->cmdBuf.write;
    imesa->cmdBuf.write += qwords;
    return ret;
}

/* Allocate Elts:
 * - if it doesn't fit, flush the cmd buffer first
 * - allocates the drawing command on the cmd buffer if there is no
 *   incomplete indexed drawing command yet
 * - increments the number of elts. Final allocation is done in savageFlushElts
 */
static __inline
u_int16_t *savageAllocElts( savageContextPtr imesa, GLuint n )
{
    u_int16_t *ret;
    GLuint qwords;
    assert (savageHaveIndexedVerts(imesa));

    if (imesa->elts.cmd)
	qwords = (imesa->elts.n + n + 3) >> 2;
    else
	qwords = ((n + 3) >> 2) + 1;
    if (imesa->cmdBuf.write - imesa->cmdBuf.base + qwords > imesa->cmdBuf.size)
	savageFlushCmdBuf(imesa, GL_FALSE); /* implies savageFlushElts */

    if (!imesa->elts.cmd) {
	savageFlushVertices(imesa);
	imesa->elts.cmd = savageAllocCmdBuf(imesa, 0);
	imesa->elts.cmd->idx.cmd = (imesa->vtxBuf == &imesa->dmaVtxBuf) ?
	    SAVAGE_CMD_DMA_IDX : SAVAGE_CMD_VB_IDX;
	imesa->elts.cmd->idx.prim = imesa->HwPrim;
	imesa->elts.cmd->idx.skip = imesa->skip;
	imesa->elts.n = 0;
    }

    ret = (u_int16_t *)(imesa->elts.cmd+1) + imesa->elts.n;
    imesa->elts.n += n;
    return ret;
}

#endif
