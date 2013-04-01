
#ifndef U_TRANSFER_H
#define U_TRANSFER_H

/* Fallback implementations for inline read/writes which just go back
 * to the regular transfer behaviour.
 */
#include "pipe/p_state.h"

struct pipe_context;
struct winsys_handle;

boolean u_default_resource_get_handle(struct pipe_screen *screen,
                                      struct pipe_resource *resource,
                                      struct winsys_handle *handle);

void u_default_transfer_inline_write( struct pipe_context *pipe,
                                      struct pipe_resource *resource,
                                      unsigned level,
                                      unsigned usage,
                                      const struct pipe_box *box,
                                      const void *data,
                                      unsigned stride,
                                      unsigned layer_stride);

void u_default_transfer_flush_region( struct pipe_context *pipe,
                                      struct pipe_transfer *transfer,
                                      const struct pipe_box *box);

struct pipe_transfer * u_default_get_transfer(struct pipe_context *context,
                                              struct pipe_resource *resource,
                                              unsigned level,
                                              unsigned usage,
                                              const struct pipe_box *box);

void u_default_transfer_unmap( struct pipe_context *pipe,
                               struct pipe_transfer *transfer );

void u_default_transfer_destroy(struct pipe_context *pipe,
                                struct pipe_transfer *transfer);



/* Useful helper to allow >1 implementation of resource functionality
 * to exist in a single driver.  This is intended to be transitionary!
 */
struct u_resource_vtbl {

   boolean (*resource_get_handle)(struct pipe_screen *,
                                  struct pipe_resource *tex,
                                  struct winsys_handle *handle);

   void (*resource_destroy)(struct pipe_screen *,
                            struct pipe_resource *pt);

   struct pipe_transfer *(*get_transfer)(struct pipe_context *,
                                         struct pipe_resource *resource,
                                         unsigned level,
                                         unsigned usage,
                                         const struct pipe_box *);

   void (*transfer_destroy)(struct pipe_context *,
                            struct pipe_transfer *);

   void *(*transfer_map)( struct pipe_context *,
                          struct pipe_transfer *transfer );

   void (*transfer_flush_region)( struct pipe_context *,
                                  struct pipe_transfer *transfer,
                                  const struct pipe_box *);

   void (*transfer_unmap)( struct pipe_context *,
   struct pipe_transfer *transfer );

   void (*transfer_inline_write)( struct pipe_context *pipe,
                                  struct pipe_resource *resource,
                                  unsigned level,
                                  unsigned usage,
                                  const struct pipe_box *box,
                                  const void *data,
                                  unsigned stride,
                                  unsigned layer_stride);
};


struct u_resource {
   struct pipe_resource b;
   const struct u_resource_vtbl *vtbl;
};


boolean u_resource_get_handle_vtbl(struct pipe_screen *screen,
                                   struct pipe_resource *resource,
                                   struct winsys_handle *handle);

void u_resource_destroy_vtbl(struct pipe_screen *screen,
                             struct pipe_resource *resource);

struct pipe_transfer *u_get_transfer_vtbl(struct pipe_context *context,
                                          struct pipe_resource *resource,
                                          unsigned level,
                                          unsigned usage,
                                          const struct pipe_box *box);

void u_transfer_destroy_vtbl(struct pipe_context *pipe,
                             struct pipe_transfer *transfer);

void *u_transfer_map_vtbl( struct pipe_context *pipe,
                           struct pipe_transfer *transfer );

void u_transfer_flush_region_vtbl( struct pipe_context *pipe,
                                   struct pipe_transfer *transfer,
                                   const struct pipe_box *box);

void u_transfer_unmap_vtbl( struct pipe_context *rm_ctx,
                            struct pipe_transfer *transfer );

void u_transfer_inline_write_vtbl( struct pipe_context *rm_ctx,
                                   struct pipe_resource *resource,
                                   unsigned level,
                                   unsigned usage,
                                   const struct pipe_box *box,
                                   const void *data,
                                   unsigned stride,
                                   unsigned layer_stride);

void u_default_redefine_user_buffer(struct pipe_context *ctx,
                                    struct pipe_resource *resource,
                                    unsigned offset,
                                    unsigned size);

#endif
