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

#ifndef UTIL_INDEX_MODIFY_H
#define UTIL_INDEX_MODIFY_H

struct pipe_context;
struct pipe_resource;

void util_shorten_ubyte_elts_to_userptr(struct pipe_context *context,
					struct pipe_resource *elts,
					int index_bias,
					unsigned start,
					unsigned count,
					void *out);

void util_shorten_ubyte_elts(struct pipe_context *context,
			     struct pipe_resource **elts,
			     int index_bias,
			     unsigned start,
			     unsigned count);



void util_rebuild_ushort_elts_to_userptr(struct pipe_context *context,
					 struct pipe_resource *elts,
					 int index_bias,
					 unsigned start, unsigned count,
					 void *out);

void util_rebuild_ushort_elts(struct pipe_context *context,
			      struct pipe_resource **elts,
			      int index_bias,
			      unsigned start, unsigned count);



void util_rebuild_uint_elts_to_userptr(struct pipe_context *context,
				       struct pipe_resource *elts,
				       int index_bias,
				       unsigned start, unsigned count,
				       void *out);

void util_rebuild_uint_elts(struct pipe_context *context,
			    struct pipe_resource **elts,
			    int index_bias,
			    unsigned start, unsigned count);

#endif
