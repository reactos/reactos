#ifndef _LINUX_VIRTIO_H
#define _LINUX_VIRTIO_H

#include "virtio_ring.h"

#define scatterlist VirtIOBufferDescriptor

struct VirtIOBufferDescriptor {
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
};

typedef int (*proc_virtqueue_add_buf)(
    struct virtqueue *vq,
    struct scatterlist sg[],
    unsigned int out_num,
    unsigned int in_num,
    void *opaque,
    void *va_indirect,
    ULONGLONG phys_indirect);

typedef bool(*proc_virtqueue_kick_prepare)(struct virtqueue *vq);

typedef void(*proc_virtqueue_kick_always)(struct virtqueue *vq);

typedef void * (*proc_virtqueue_get_buf)(struct virtqueue *vq, unsigned int *len);

typedef void(*proc_virtqueue_disable_cb)(struct virtqueue *vq);

typedef bool(*proc_virtqueue_enable_cb)(struct virtqueue *vq);

typedef bool(*proc_virtqueue_enable_cb_delayed)(struct virtqueue *vq);

typedef void * (*proc_virtqueue_detach_unused_buf)(struct virtqueue *vq);

typedef BOOLEAN(*proc_virtqueue_is_interrupt_enabled)(struct virtqueue *vq);

typedef BOOLEAN(*proc_virtqueue_has_buf)(struct virtqueue *vq);

typedef void(*proc_virtqueue_shutdown)(struct virtqueue *vq);

/* Represents one virtqueue; only data pointed to by the vring structure is exposed to the host */
struct virtqueue {
    VirtIODevice *vdev;
    unsigned int index;
    void (*notification_cb)(struct virtqueue *vq);
    void         *notification_addr;
    void         *avail_va;
    void         *used_va;
    proc_virtqueue_add_buf add_buf;
    proc_virtqueue_kick_prepare kick_prepare;
    proc_virtqueue_kick_always kick_always;
    proc_virtqueue_get_buf get_buf;
    proc_virtqueue_disable_cb disable_cb;
    proc_virtqueue_enable_cb enable_cb;
    proc_virtqueue_enable_cb_delayed enable_cb_delayed;
    proc_virtqueue_detach_unused_buf detach_unused_buf;
    proc_virtqueue_is_interrupt_enabled is_interrupt_enabled;
    proc_virtqueue_has_buf has_buf;
    proc_virtqueue_shutdown shutdown;
};

static inline int virtqueue_add_buf(
    struct virtqueue *vq,
    struct scatterlist sg[],
    unsigned int out_num,
    unsigned int in_num,
    void *opaque,
    void *va_indirect,
    ULONGLONG phys_indirect)
{
    return vq->add_buf(vq, sg, out_num, in_num, opaque, va_indirect, phys_indirect);
}

static inline bool virtqueue_kick_prepare(struct virtqueue *vq)
{
    return vq->kick_prepare(vq);
}

static inline void virtqueue_kick_always(struct virtqueue *vq)
{
    vq->kick_always(vq);
}

static inline void *virtqueue_get_buf(struct virtqueue *vq, unsigned int *len)
{
    return vq->get_buf(vq, len);
}

static inline void virtqueue_disable_cb(struct virtqueue *vq)
{
    vq->disable_cb(vq);
}

static inline bool virtqueue_enable_cb(struct virtqueue *vq)
{
    return vq->enable_cb(vq);
}

static inline bool virtqueue_enable_cb_delayed(struct virtqueue *vq)
{
    return vq->enable_cb_delayed(vq);
}

static inline void *virtqueue_detach_unused_buf(struct virtqueue *vq)
{
    return vq->detach_unused_buf(vq);
}

static inline BOOLEAN virtqueue_is_interrupt_enabled(struct virtqueue *vq)
{
    return vq->is_interrupt_enabled(vq);
}

static inline BOOLEAN virtqueue_has_buf(struct virtqueue *vq)
{
    return vq->has_buf(vq);
}

static inline void virtqueue_shutdown(struct virtqueue *vq)
{
    vq->shutdown(vq);
}

void virtqueue_notify(struct virtqueue *vq);
void virtqueue_kick(struct virtqueue *vq);

#endif /* _LINUX_VIRTIO_H */
