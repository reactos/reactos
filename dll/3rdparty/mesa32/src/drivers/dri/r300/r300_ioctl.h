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

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#ifndef __R300_IOCTL_H__
#define __R300_IOCTL_H__

#include "r300_context.h"
#include "radeon_drm.h"

extern GLboolean r300IsGartMemory(r300ContextPtr rmesa,
				  const GLvoid * pointer, GLint size);

extern GLuint r300GartOffsetFromVirtual(r300ContextPtr rmesa,
					const GLvoid * pointer);

extern void r300Flush(GLcontext * ctx);

extern void r300ReleaseDmaRegion(r300ContextPtr rmesa,
				 struct r300_dma_region *region,
				 const char *caller);
extern void r300AllocDmaRegion(r300ContextPtr rmesa,
			       struct r300_dma_region *region, int bytes,
			       int alignment);

extern void r300InitIoctlFuncs(struct dd_function_table *functions);

#endif				/* __R300_IOCTL_H__ */
