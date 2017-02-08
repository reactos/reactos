/**************************************************************************
 *
 * Copyright 2009-2010 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef U_FRAMEBUFFER_H
#define U_FRAMEBUFFER_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"


extern boolean
util_framebuffer_state_equal(const struct pipe_framebuffer_state *dst,
                             const struct pipe_framebuffer_state *src);

extern void
util_copy_framebuffer_state(struct pipe_framebuffer_state *dst,
                            const struct pipe_framebuffer_state *src);


extern void
util_unreference_framebuffer_state(struct pipe_framebuffer_state *fb);


extern boolean
util_framebuffer_min_size(const struct pipe_framebuffer_state *fb,
                          unsigned *width,
                          unsigned *height);

#endif /* U_FRAMEBUFFER_H */
