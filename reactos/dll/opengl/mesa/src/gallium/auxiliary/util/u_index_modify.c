/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "pipe/p_context.h"
#include "util/u_index_modify.h"
#include "util/u_inlines.h"

/* Ubyte indices. */

void util_shorten_ubyte_elts_to_userptr(struct pipe_context *context,
					struct pipe_resource *elts,
					int index_bias,
					unsigned start,
					unsigned count,
					void *out)
{
    struct pipe_transfer *src_transfer;
    unsigned char *in_map;
    unsigned short *out_map = out;
    unsigned i;

    in_map = pipe_buffer_map(context, elts,
                             PIPE_TRANSFER_READ |
                             PIPE_TRANSFER_UNSYNCHRONIZED,
                             &src_transfer);
    in_map += start;

    for (i = 0; i < count; i++) {
        *out_map = (unsigned short)(*in_map + index_bias);
        in_map++;
        out_map++;
    }

    pipe_buffer_unmap(context, src_transfer);
}

void util_shorten_ubyte_elts(struct pipe_context *context,
			     struct pipe_resource **elts,
			     int index_bias,
			     unsigned start,
			     unsigned count)
{
    struct pipe_resource* new_elts;
    unsigned short *out_map;
    struct pipe_transfer *dst_transfer;

    new_elts = pipe_buffer_create(context->screen,
                                  PIPE_BIND_INDEX_BUFFER,
                                  PIPE_USAGE_STATIC,
                                  2 * count);

    out_map = pipe_buffer_map(context, new_elts, PIPE_TRANSFER_WRITE,
                              &dst_transfer);
    util_shorten_ubyte_elts_to_userptr(context, *elts, index_bias,
                                       start, count, out_map);
    pipe_buffer_unmap(context, dst_transfer);

    *elts = new_elts;
}


/* Ushort indices. */

void util_rebuild_ushort_elts_to_userptr(struct pipe_context *context,
					 struct pipe_resource *elts,
					 int index_bias,
					 unsigned start, unsigned count,
					 void *out)
{
    struct pipe_transfer *in_transfer = NULL;
    unsigned short *in_map;
    unsigned short *out_map = out;
    unsigned i;

    in_map = pipe_buffer_map(context, elts,
                             PIPE_TRANSFER_READ |
                             PIPE_TRANSFER_UNSYNCHRONIZED,
                             &in_transfer);
    in_map += start;

    for (i = 0; i < count; i++) {
        *out_map = (unsigned short)(*in_map + index_bias);
        in_map++;
        out_map++;
    }

    pipe_buffer_unmap(context, in_transfer);
}

void util_rebuild_ushort_elts(struct pipe_context *context,
			      struct pipe_resource **elts,
			      int index_bias,
			      unsigned start, unsigned count)
{
    struct pipe_transfer *out_transfer = NULL;
    struct pipe_resource *new_elts;
    unsigned short *out_map;

    new_elts = pipe_buffer_create(context->screen,
                                  PIPE_BIND_INDEX_BUFFER,
                                  PIPE_USAGE_STATIC,
                                  2 * count);

    out_map = pipe_buffer_map(context, new_elts,
                              PIPE_TRANSFER_WRITE, &out_transfer);
    util_rebuild_ushort_elts_to_userptr(context, *elts, index_bias,
                                        start, count, out_map);
    pipe_buffer_unmap(context, out_transfer);

    *elts = new_elts;
}


/* Uint indices. */

void util_rebuild_uint_elts_to_userptr(struct pipe_context *context,
				       struct pipe_resource *elts,
				       int index_bias,
				       unsigned start, unsigned count,
				       void *out)
{
    struct pipe_transfer *in_transfer = NULL;
    unsigned int *in_map;
    unsigned int *out_map = out;
    unsigned i;

    in_map = pipe_buffer_map(context, elts,
                             PIPE_TRANSFER_READ |
                             PIPE_TRANSFER_UNSYNCHRONIZED,
                             &in_transfer);
    in_map += start;

    for (i = 0; i < count; i++) {
        *out_map = (unsigned int)(*in_map + index_bias);
        in_map++;
        out_map++;
    }

    pipe_buffer_unmap(context, in_transfer);
}

void util_rebuild_uint_elts(struct pipe_context *context,
			    struct pipe_resource **elts,
			    int index_bias,
			    unsigned start, unsigned count)
{
    struct pipe_transfer *out_transfer = NULL;
    struct pipe_resource *new_elts;
    unsigned int *out_map;

    new_elts = pipe_buffer_create(context->screen,
                                  PIPE_BIND_INDEX_BUFFER,
                                  PIPE_USAGE_STATIC,
                                  2 * count);

    out_map = pipe_buffer_map(context, new_elts,
                              PIPE_TRANSFER_WRITE, &out_transfer);
    util_rebuild_uint_elts_to_userptr(context, *elts, index_bias,
                                      start, count, out_map);
    pipe_buffer_unmap(context, out_transfer);

    *elts = new_elts;
}
