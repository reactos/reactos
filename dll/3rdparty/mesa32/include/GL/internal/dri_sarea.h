/*
 * Copyright 2007 Red Hat, Inc
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DRI_SAREA_H
#define DRI_SAREA_H

#include <drm.h>

/* The DRI2 SAREA holds a list of self-describing blocks.  Each block
 * is 8 byte aligned and has a common 32-bit header word.  The upper
 * 16 bits describe the type of the block and the lower 16 bits the
 * size.  DRI2 only defines a couple of blocks and allows drivers to
 * define driver specific blocks using type codes from 0x8000 and up.
 * The type code 0x0000 defines the end of the sarea. */

#define DRI2_SAREA_BLOCK_HEADER(type, size) (((type) << 16) | (size))
#define DRI2_SAREA_BLOCK_TYPE(b) ((b) >> 16)
#define DRI2_SAREA_BLOCK_SIZE(b) ((b) & 0xffff)
#define DRI2_SAREA_BLOCK_NEXT(p)				\
    ((void *) ((unsigned char *) (p) +				\
               DRI2_SAREA_BLOCK_SIZE(*(unsigned int *) p)))

#define DRI2_SAREA_BLOCK_END		0x0000
#define DRI2_SAREA_BLOCK_LOCK		0x0001
#define DRI2_SAREA_BLOCK_EVENT_BUFFER	0x0002

/* Chipset specific blocks start at 0x8000, 0xffff is reserved. */

typedef struct __DRILock __DRILock;
typedef struct __DRIEventBuffer __DRIEventBuffer;
typedef struct __DRIDrawableBuffer __DRIDrawableBuffer;
typedef struct __DRIDrawableConfigEvent __DRIDrawableConfigEvent;
typedef struct __DRIBufferAttachEvent __DRIBufferAttachEvent;

struct __DRILock {
    unsigned int block_header;
    drm_hw_lock_t lock;

    /* We use this with DRM_CAS to allocate lock IDs for the real lock.*/
    unsigned int next_id;
};

struct __DRIEventBuffer {
    unsigned int block_header;
    unsigned int head;		/* last valid event */
    unsigned int prealloc;	/* event currently being written */
    unsigned int size;		/* size of data */
    unsigned char data[0];
};

enum {
        /* the four standard color buffers */
        DRI_DRAWABLE_BUFFER_FRONT_LEFT  = 0,
        DRI_DRAWABLE_BUFFER_BACK_LEFT   = 1,
        DRI_DRAWABLE_BUFFER_FRONT_RIGHT = 2,
        DRI_DRAWABLE_BUFFER_BACK_RIGHT  = 3,
        /* optional aux buffer */
        DRI_DRAWABLE_BUFFER_AUX0        = 4,
        DRI_DRAWABLE_BUFFER_AUX1        = 5,
        DRI_DRAWABLE_BUFFER_AUX2        = 6,
        DRI_DRAWABLE_BUFFER_AUX3        = 7,
        DRI_DRAWABLE_BUFFER_DEPTH       = 8,
        DRI_DRAWABLE_BUFFER_STENCIL     = 9,
        DRI_DRAWABLE_BUFFER_ACCUM       = 10,
        /* generic renderbuffers */
        DRI_DRAWABLE_BUFFER_COLOR0      = 11,
        DRI_DRAWABLE_BUFFER_COLOR1      = 12,
        DRI_DRAWABLE_BUFFER_COLOR2      = 13,
        DRI_DRAWABLE_BUFFER_COLOR3      = 14,
        DRI_DRAWABLE_BUFFER_COLOR4      = 15,
        DRI_DRAWABLE_BUFFER_COLOR5      = 16,
        DRI_DRAWABLE_BUFFER_COLOR6      = 17,
        DRI_DRAWABLE_BUFFER_COLOR7      = 18,
        DRI_DRAWABLE_BUFFER_COUNT       = 19
};

struct __DRIDrawableBuffer {
    unsigned int attachment;
    unsigned int handle;
    unsigned int pitch;
    unsigned short cpp;

    /* Upper 8 bits are driver specific, lower 8 bits generic.  The
     * bits can inidicate buffer properties such as tiled, swizzled etc. */
    unsigned short flags;
};

#define DRI2_EVENT_HEADER(type, size) (((type) << 16) | (size))
#define DRI2_EVENT_TYPE(b) ((b) >> 16)
#define DRI2_EVENT_SIZE(b) ((b) & 0xffff)

#define DRI2_EVENT_PAD			0x0000
#define DRI2_EVENT_DRAWABLE_CONFIG	0x0001
#define DRI2_EVENT_BUFFER_ATTACH	0x0002

struct __DRIDrawableConfigEvent {
    unsigned int		event_header;
    unsigned int		drawable;
    short			x;
    short			y;
    unsigned int		width;
    unsigned int		height;
    unsigned int		num_rects;
    struct drm_clip_rect	rects[0];
};

struct __DRIBufferAttachEvent {
    unsigned int	event_header;
    unsigned int	drawable;
    __DRIDrawableBuffer	buffer;
};

#endif /* DRI_SAREA_H */
