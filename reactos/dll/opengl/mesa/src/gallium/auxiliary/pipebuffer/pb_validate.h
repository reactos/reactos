/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * @file
 * Buffer validation.
 * 
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef PB_VALIDATE_H_
#define PB_VALIDATE_H_


#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"

#ifdef __cplusplus
extern "C" {
#endif


struct pb_buffer;
struct pipe_fence_handle;


/**
 * Buffer validation list.
 * 
 * It holds a list of buffers to be validated and fenced when flushing.
 */
struct pb_validate;


enum pipe_error
pb_validate_add_buffer(struct pb_validate *vl,
                       struct pb_buffer *buf,
                       unsigned flags);

enum pipe_error
pb_validate_foreach(struct pb_validate *vl,
                    enum pipe_error (*callback)(struct pb_buffer *buf, void *data),
                    void *data);

/**
 * Validate all buffers for hardware access.
 * 
 * Should be called right before issuing commands to the hardware.
 */
enum pipe_error
pb_validate_validate(struct pb_validate *vl);

/**
 * Fence all buffers and clear the list.
 * 
 * Should be called right after issuing commands to the hardware.
 */
void
pb_validate_fence(struct pb_validate *vl,
                  struct pipe_fence_handle *fence);

struct pb_validate *
pb_validate_create(void);

void
pb_validate_destroy(struct pb_validate *vl);


#ifdef __cplusplus
}
#endif

#endif /*PB_VALIDATE_H_*/
