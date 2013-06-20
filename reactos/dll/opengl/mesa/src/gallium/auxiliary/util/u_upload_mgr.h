/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Helper utility for uploading user buffers & other data, and
 * coalescing small buffers into larger ones.
 */

#ifndef U_UPLOAD_MGR_H
#define U_UPLOAD_MGR_H

#include "pipe/p_compiler.h"

struct pipe_context;
struct pipe_resource;


/**
 * Create the upload manager.
 *
 * \param pipe          Pipe driver.
 * \param default_size  Minimum size of the upload buffer, in bytes.
 * \param alignment     Alignment of each suballocation in the upload buffer.
 * \param bind          Bitmask of PIPE_BIND_* flags.
 */
struct u_upload_mgr *u_upload_create( struct pipe_context *pipe,
                                      unsigned default_size,
                                      unsigned alignment,
                                      unsigned bind );

/**
 * Destroy the upload manager.
 */
void u_upload_destroy( struct u_upload_mgr *upload );

/* Unmap and release old upload buffer.
 * 
 * This is like u_upload_unmap() except the upload buffer is released for
 * recycling. This should be called on real hardware flushes on systems
 * that don't support the PIPE_TRANSFER_UNSYNCHRONIZED flag, as otherwise
 * the next u_upload_buffer will cause a sync on the buffer.
 */

void u_upload_flush( struct u_upload_mgr *upload );

/**
 * Unmap upload buffer
 *
 * \param upload           Upload manager
 *
 * This must usually be called prior to firing the command stream
 * which references the upload buffer, as many memory managers either
 * don't like firing a mapped buffer or cause subsequent maps of a
 * fired buffer to wait.
 */
void u_upload_unmap( struct u_upload_mgr *upload );

/**
 * Sub-allocate new memory from the upload buffer.
 *
 * \param upload           Upload manager
 * \param min_out_offset   Minimum offset that should be returned in out_offset.
 * \param size             Size of the allocation.
 * \param out_offset       Pointer to where the new buffer offset will be returned.
 * \param outbuf           Pointer to where the upload buffer will be returned.
 * \param flushed          Whether the upload buffer was flushed.
 * \param ptr              Pointer to the allocated memory that is returned.
 */
enum pipe_error u_upload_alloc( struct u_upload_mgr *upload,
                                unsigned min_out_offset,
                                unsigned size,
                                unsigned *out_offset,
                                struct pipe_resource **outbuf,
                                void **ptr );


/**
 * Allocate and write data to the upload buffer.
 *
 * Same as u_upload_alloc, but in addition to that, it copies "data"
 * to the pointer returned from u_upload_alloc.
 */
enum pipe_error u_upload_data( struct u_upload_mgr *upload,
                               unsigned min_out_offset,
                               unsigned size,
                               const void *data,
                               unsigned *out_offset,
                               struct pipe_resource **outbuf);


/**
 * Allocate and copy an input buffer to the upload buffer.
 *
 * Same as u_upload_data, except that the input data comes from a buffer
 * instead of a user pointer.
 */
enum pipe_error u_upload_buffer( struct u_upload_mgr *upload,
                                 unsigned min_out_offset,
                                 unsigned offset,
                                 unsigned size,
                                 struct pipe_resource *inbuf,
                                 unsigned *out_offset,
                                 struct pipe_resource **outbuf);



#endif

