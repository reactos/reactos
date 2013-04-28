
#ifndef UTIL_RINGBUFFER_H
#define UTIL_RINGBUFFER_H

#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"       /* only for pipe_error! */

/* Generic header
 */
struct util_packet {
   unsigned dwords:8;
   unsigned data24:24;
};

struct util_ringbuffer;

struct util_ringbuffer *util_ringbuffer_create( unsigned dwords );

void util_ringbuffer_destroy( struct util_ringbuffer *ring );

void util_ringbuffer_enqueue( struct util_ringbuffer *ring,
                              const struct util_packet *packet );

enum pipe_error util_ringbuffer_dequeue( struct util_ringbuffer *ring,
                                         struct util_packet *packet,
                                         unsigned max_dwords,
                                         boolean wait );

#endif
