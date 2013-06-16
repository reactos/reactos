

#include "util/u_inlines.h"
#include "util/u_transfer.h"

static INLINE struct u_resource *
u_resource( struct pipe_resource *res )
{
   return (struct u_resource *)res;
}

boolean u_resource_get_handle_vtbl(struct pipe_screen *screen,
                                   struct pipe_resource *resource,
                                   struct winsys_handle *handle)
{
   struct u_resource *ur = u_resource(resource);
   return ur->vtbl->resource_get_handle(screen, resource, handle);
}

void u_resource_destroy_vtbl(struct pipe_screen *screen,
                             struct pipe_resource *resource)
{
   struct u_resource *ur = u_resource(resource);
   ur->vtbl->resource_destroy(screen, resource);
}

struct pipe_transfer *u_get_transfer_vtbl(struct pipe_context *context,
                                          struct pipe_resource *resource,
                                          unsigned level,
                                          unsigned usage,
                                          const struct pipe_box *box)
{
   struct u_resource *ur = u_resource(resource);
   return ur->vtbl->get_transfer(context, resource, level, usage, box);
}

void u_transfer_destroy_vtbl(struct pipe_context *pipe,
                             struct pipe_transfer *transfer)
{
   struct u_resource *ur = u_resource(transfer->resource);
   ur->vtbl->transfer_destroy(pipe, transfer);
}

void *u_transfer_map_vtbl( struct pipe_context *pipe,
                           struct pipe_transfer *transfer )
{
   struct u_resource *ur = u_resource(transfer->resource);
   return ur->vtbl->transfer_map(pipe, transfer);
}

void u_transfer_flush_region_vtbl( struct pipe_context *pipe,
                                   struct pipe_transfer *transfer,
                                   const struct pipe_box *box)
{
   struct u_resource *ur = u_resource(transfer->resource);
   ur->vtbl->transfer_flush_region(pipe, transfer, box);
}

void u_transfer_unmap_vtbl( struct pipe_context *pipe,
                            struct pipe_transfer *transfer )
{
   struct u_resource *ur = u_resource(transfer->resource);
   ur->vtbl->transfer_unmap(pipe, transfer);
}

void u_transfer_inline_write_vtbl( struct pipe_context *pipe,
                                   struct pipe_resource *resource,
                                   unsigned level,
                                   unsigned usage,
                                   const struct pipe_box *box,
                                   const void *data,
                                   unsigned stride,
                                   unsigned layer_stride)
{
   struct u_resource *ur = u_resource(resource);
   ur->vtbl->transfer_inline_write(pipe,
                                   resource,
                                   level,
                                   usage,
                                   box,
                                   data,
                                   stride,
                                   layer_stride);
}




