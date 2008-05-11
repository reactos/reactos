/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/**
 * \file
 *
 * \author Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R300_CMDBUF_H__
#define __R300_CMDBUF_H__

#include "r300_context.h"

extern int r300FlushCmdBufLocked(r300ContextPtr r300, const char *caller);
extern int r300FlushCmdBuf(r300ContextPtr r300, const char *caller);

extern void r300EmitState(r300ContextPtr r300);

extern void r300InitCmdBuf(r300ContextPtr r300);
extern void r300DestroyCmdBuf(r300ContextPtr r300);

/**
 * Make sure that enough space is available in the command buffer
 * by flushing if necessary.
 *
 * \param dwords The number of dwords we need to be free on the command buffer
 */
static __inline__ void r300EnsureCmdBufSpace(r300ContextPtr r300,
					     int dwords, const char *caller)
{
	assert(dwords < r300->cmdbuf.size);

	if (r300->cmdbuf.count_used + dwords > r300->cmdbuf.size)
		r300FlushCmdBuf(r300, caller);
}

/**
 * Allocate the given number of dwords in the command buffer and return
 * a pointer to the allocated area.
 * When necessary, these functions cause a flush. r300AllocCmdBuf() also
 * causes state reemission after a flush. This is necessary to ensure
 * correct hardware state after an unlock.
 */
static __inline__ uint32_t *r300RawAllocCmdBuf(r300ContextPtr r300,
					       int dwords, const char *caller)
{
	uint32_t *ptr;

	r300EnsureCmdBufSpace(r300, dwords, caller);

	ptr = &r300->cmdbuf.cmd_buf[r300->cmdbuf.count_used];
	r300->cmdbuf.count_used += dwords;
	return ptr;
}

static __inline__ uint32_t *r300AllocCmdBuf(r300ContextPtr r300,
					    int dwords, const char *caller)
{
	uint32_t *ptr;

	r300EnsureCmdBufSpace(r300, dwords, caller);

	if (!r300->cmdbuf.count_used) {
		if (RADEON_DEBUG & DEBUG_IOCTL)
			fprintf(stderr,
				"Reemit state after flush (from %s)\n", caller);
		r300EmitState(r300);
	}

	ptr = &r300->cmdbuf.cmd_buf[r300->cmdbuf.count_used];
	r300->cmdbuf.count_used += dwords;
	return ptr;
}

extern void r300EmitBlit(r300ContextPtr rmesa,
			 GLuint color_fmt,
			 GLuint src_pitch,
			 GLuint src_offset,
			 GLuint dst_pitch,
			 GLuint dst_offset,
			 GLint srcx, GLint srcy,
			 GLint dstx, GLint dsty, GLuint w, GLuint h);

extern void r300EmitWait(r300ContextPtr rmesa, GLuint flags);
extern void r300EmitLOAD_VBPNTR(r300ContextPtr rmesa, int start);
extern void r300EmitVertexShader(r300ContextPtr rmesa);
extern void r300EmitPixelShader(r300ContextPtr rmesa);

#endif				/* __R300_CMDBUF_H__ */
