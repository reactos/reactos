/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/
/*
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 *          Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#ifndef _DRI_BUFMGR_H_
#define _DRI_BUFMGR_H_
#include <xf86drm.h>


struct _DriFenceObject;
struct _DriBufferObject;
struct _DriBufferPool;

extern struct _DriFenceObject *driFenceBuffers(int fd, char *name,
                                               unsigned flags);

extern struct _DriFenceObject *driFenceReference(struct _DriFenceObject *fence);

extern void driFenceUnReference(struct _DriFenceObject *fence);

extern void
driFenceFinish(struct _DriFenceObject *fence, unsigned type, int lazy);

extern int driFenceSignaled(struct _DriFenceObject *fence, unsigned type);
extern unsigned driFenceType(struct _DriFenceObject *fence);

/*
 * Return a pointer to the libdrm buffer object this DriBufferObject
 * uses.
 */

extern drmBO *driBOKernel(struct _DriBufferObject *buf);
extern void *driBOMap(struct _DriBufferObject *buf, unsigned flags,
                      unsigned hint);
extern void driBOUnmap(struct _DriBufferObject *buf);
extern unsigned long driBOOffset(struct _DriBufferObject *buf);
extern unsigned driBOFlags(struct _DriBufferObject *buf);
extern struct _DriBufferObject *driBOReference(struct _DriBufferObject *buf);
extern void driBOUnReference(struct _DriBufferObject *buf);
extern void driBOData(struct _DriBufferObject *r_buf,
                      unsigned size, const void *data, unsigned flags);
extern void driBOSubData(struct _DriBufferObject *buf,
                         unsigned long offset, unsigned long size,
                         const void *data);
extern void driBOGetSubData(struct _DriBufferObject *buf,
                            unsigned long offset, unsigned long size,
                            void *data);
extern void driGenBuffers(struct _DriBufferPool *pool,
                          const char *name,
                          unsigned n,
                          struct _DriBufferObject *buffers[],
                          unsigned alignment, unsigned flags, unsigned hint);
extern void driDeleteBuffers(unsigned n, struct _DriBufferObject *buffers[]);
extern void driInitBufMgr(int fd);
extern void driBOCreateList(int target, drmBOList * list);
extern void driBOResetList(drmBOList * list);
extern void driBOAddListItem(drmBOList * list, struct _DriBufferObject *buf,
                             unsigned flags, unsigned mask);
extern void driBOValidateList(int fd, drmBOList * list);

extern void driBOFence(struct _DriBufferObject *buf,
                       struct _DriFenceObject *fence);

extern void driPoolTakeDown(struct _DriBufferPool *pool);
extern void driBOSetStatic(struct _DriBufferObject *buf,
                           unsigned long offset,
                           unsigned long size, void *virtual, unsigned flags);
extern void driBOWaitIdle(struct _DriBufferObject *buf, int lazy);
extern void driPoolTakeDown(struct _DriBufferPool *pool);

#endif
