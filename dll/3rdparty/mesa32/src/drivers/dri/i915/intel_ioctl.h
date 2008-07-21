/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTEL_IOCTL_H
#define INTEL_IOCTL_H

#include "intel_context.h"

extern void intelWaitAgeLocked( intelContextPtr intel, int age, GLboolean unlock );

extern void intelClear(GLcontext *ctx, GLbitfield mask);

extern void intelPageFlip( const __DRIdrawablePrivate *dpriv );

extern void intelRotateWindow(intelContextPtr intel,
                              __DRIdrawablePrivate *dPriv, GLuint srcBuffer);

extern void intelWaitForIdle( intelContextPtr intel );
extern void intelFlushBatch( intelContextPtr intel, GLboolean refill );
extern void intelFlushBatchLocked( intelContextPtr intel,
				   GLboolean ignore_cliprects,
				   GLboolean refill,
				   GLboolean allow_unlock);
extern void intelRefillBatchLocked( intelContextPtr intel, GLboolean allow_unlock );
extern void intelFinish( GLcontext *ctx );
extern void intelFlush( GLcontext *ctx );
extern void intelglFlush( GLcontext *ctx );

extern void *intelAllocateAGP( intelContextPtr intel, GLsizei size );
extern void intelFreeAGP( intelContextPtr intel, void *pointer );

extern void *intelAllocateMemoryMESA( __DRInativeDisplay *dpy, int scrn, 
				      GLsizei size, GLfloat readfreq,
				      GLfloat writefreq, GLfloat priority );

extern void intelFreeMemoryMESA( __DRInativeDisplay *dpy, int scrn, 
				 GLvoid *pointer );

extern GLuint intelGetMemoryOffsetMESA( __DRInativeDisplay *dpy, int scrn, const GLvoid *pointer );
extern GLboolean intelIsAgpMemory( intelContextPtr intel, const GLvoid *pointer,
				  GLint size );

extern GLuint intelAgpOffsetFromVirtual( intelContextPtr intel, const GLvoid *p );

extern void intelWaitIrq( intelContextPtr intel, int seq );
extern u_int32_t intelGetLastFrame (intelContextPtr intel);
extern int intelEmitIrqLocked( intelContextPtr intel );
#endif
