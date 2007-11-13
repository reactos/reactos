/*
 * Copyright (C) 2007 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __NOUVEAU_SYNC_H__
#define __NOUVEAU_SYNC_H__

#include "nouveau_buffers.h"

#define NV_NOTIFIER_SIZE                                                      32
#define NV_NOTIFY_TIME_0                                              0x00000000
#define NV_NOTIFY_TIME_1                                              0x00000004
#define NV_NOTIFY_RETURN_VALUE                                        0x00000008
#define NV_NOTIFY_STATE                                               0x0000000C
#define NV_NOTIFY_STATE_STATUS_MASK                                   0xFF000000
#define NV_NOTIFY_STATE_STATUS_SHIFT                                          24
#define NV_NOTIFY_STATE_STATUS_COMPLETED                                    0x00
#define NV_NOTIFY_STATE_STATUS_IN_PROCESS                                   0x01
#define NV_NOTIFY_STATE_ERROR_CODE_MASK                               0x0000FFFF
#define NV_NOTIFY_STATE_ERROR_CODE_SHIFT                                       0

/* Methods that (hopefully) all objects have */
#define NV_NOP                                                        0x00000100
#define NV_NOTIFY                                                     0x00000104
#define NV_NOTIFY_STYLE_WRITE_ONLY                                             0

typedef struct nouveau_notifier_t {
	GLuint       handle;
	nouveau_mem *mem;
} nouveau_notifier;

extern nouveau_notifier *nouveau_notifier_new(GLcontext *, GLuint handle,
					      GLuint count);
extern void nouveau_notifier_destroy(GLcontext *, nouveau_notifier *);
extern void nouveau_notifier_reset(nouveau_notifier *, GLuint id);
extern GLuint nouveau_notifier_status(nouveau_notifier *, GLuint id);
extern GLuint nouveau_notifier_return_val(nouveau_notifier *, GLuint id);
extern GLboolean nouveau_notifier_wait_status(nouveau_notifier *r, GLuint id,
					      GLuint status, GLuint timeout);
extern void nouveau_notifier_wait_nop(GLcontext *ctx,
      				      nouveau_notifier *, GLuint subc);

extern GLboolean nouveauSyncInitFuncs(GLcontext *ctx);
#endif
