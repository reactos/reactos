
#include "os/os_thread.h"
#include "pipe/p_defines.h"
#include "util/u_ringbuffer.h"
#include "util/u_math.h"
#include "util/u_memory.h"

/* Generic ringbuffer: 
 */
struct util_ringbuffer 
{
   struct util_packet *buf;
   unsigned mask;

   /* Can this be done with atomic variables??
    */
   unsigned head;
   unsigned tail;
   pipe_condvar change;
   pipe_mutex mutex;
};


struct util_ringbuffer *util_ringbuffer_create( unsigned dwords )
{
   struct util_ringbuffer *ring = CALLOC_STRUCT(util_ringbuffer);
   if (ring == NULL)
      return NULL;

   assert(util_is_power_of_two(dwords));
   
   ring->buf = MALLOC( dwords * sizeof(unsigned) );
   if (ring->buf == NULL)
      goto fail;

   ring->mask = dwords - 1;

   pipe_condvar_init(ring->change);
   pipe_mutex_init(ring->mutex);
   return ring;

fail:
   FREE(ring->buf);
   FREE(ring);
   return NULL;
}

void util_ringbuffer_destroy( struct util_ringbuffer *ring )
{
   pipe_condvar_destroy(ring->change);
   pipe_mutex_destroy(ring->mutex);
   FREE(ring->buf);
   FREE(ring);
}

/**
 * Return number of free entries in the ring
 */
static INLINE unsigned util_ringbuffer_space( const struct util_ringbuffer *ring )
{
   return (ring->tail - (ring->head + 1)) & ring->mask;
}

/**
 * Is the ring buffer empty?
 */
static INLINE boolean util_ringbuffer_empty( const struct util_ringbuffer *ring )
{
   return util_ringbuffer_space(ring) == ring->mask;
}

void util_ringbuffer_enqueue( struct util_ringbuffer *ring,
                              const struct util_packet *packet )
{
   unsigned i;

   /* XXX: over-reliance on mutexes, etc:
    */
   pipe_mutex_lock(ring->mutex);

   /* make sure we don't request an impossible amount of space
    */
   assert(packet->dwords <= ring->mask);

   /* Wait for free space:
    */
   while (util_ringbuffer_space(ring) < packet->dwords)
      pipe_condvar_wait(ring->change, ring->mutex);

   /* Copy data to ring:
    */
   for (i = 0; i < packet->dwords; i++) {

      /* Copy all dwords of the packet.  Note we're abusing the
       * typesystem a little - we're being passed a pointer to
       * something, but probably not an array of packet structs:
       */
      ring->buf[ring->head] = packet[i];
      ring->head++;
      ring->head &= ring->mask;
   }

   /* Signal change:
    */
   pipe_condvar_signal(ring->change);
   pipe_mutex_unlock(ring->mutex);
}

enum pipe_error util_ringbuffer_dequeue( struct util_ringbuffer *ring,
                                         struct util_packet *packet,
                                         unsigned max_dwords,
                                         boolean wait )
{
   const struct util_packet *ring_packet;
   unsigned i;
   int ret = PIPE_OK;

   /* XXX: over-reliance on mutexes, etc:
    */
   pipe_mutex_lock(ring->mutex);

   /* Get next ring entry:
    */
   if (wait) {
      while (util_ringbuffer_empty(ring))
         pipe_condvar_wait(ring->change, ring->mutex);
   }
   else {
      if (util_ringbuffer_empty(ring)) {
         ret = PIPE_ERROR_OUT_OF_MEMORY;
         goto out;
      }
   }

   ring_packet = &ring->buf[ring->tail];

   /* Both of these are considered bugs.  Raise an assert on debug builds.
    */
   if (ring_packet->dwords > ring->mask + 1 - util_ringbuffer_space(ring) ||
       ring_packet->dwords > max_dwords) {
      assert(0);
      ret = PIPE_ERROR_BAD_INPUT;
      goto out;
   }

   /* Copy data from ring:
    */
   for (i = 0; i < ring_packet->dwords; i++) {
      packet[i] = ring->buf[ring->tail];
      ring->tail++;
      ring->tail &= ring->mask;
   }

out:
   /* Signal change:
    */
   pipe_condvar_signal(ring->change);
   pipe_mutex_unlock(ring->mutex);
   return ret;
}
