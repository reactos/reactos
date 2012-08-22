/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
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
 **************************************************************************/

/* Direct3D 10/11 has no concept of transfers. Applications instead
 * create resources with a STAGING or DYNAMIC usage, copy between them
 * and the real resource and use Map to map the STAGING/DYNAMIC resource.
 *
 * This util module allows to implement Gallium drivers as a Direct3D
 * driver would be implemented: transfers allocate a resource with
 * PIPE_USAGE_STAGING, and copy the data between it and the real resource
 * with resource_copy_region.
 */

#ifndef U_STAGING_H
#define U_STAGING_H

#include "pipe/p_state.h"

struct util_staging_transfer {
   struct pipe_transfer base;

   /* if direct, same as base.resource, otherwise the temporary staging resource */
   struct pipe_resource *staging_resource;
};

/* user must be stride, slice_stride and offset */
/* pt->usage == PIPE_USAGE_DYNAMIC || pt->usage == PIPE_USAGE_STAGING should be a good value to pass for direct */
/* staging resource is currently created with PIPE_USAGE_STAGING */
struct util_staging_transfer *
util_staging_transfer_init(struct pipe_context *pipe,
           struct pipe_resource *pt,
           unsigned level,
           unsigned usage,
           const struct pipe_box *box,
           boolean direct, struct util_staging_transfer *tx);

void
util_staging_transfer_destroy(struct pipe_context *pipe, struct pipe_transfer *ptx);

#endif
