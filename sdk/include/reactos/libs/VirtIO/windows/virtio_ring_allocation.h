#ifndef _VIRTIO_RING_ALLOCATION_H
#define _VIRTIO_RING_ALLOCATION_H

struct virtqueue *vring_new_virtqueue(unsigned int index,
    unsigned int num,
    unsigned int vring_align,
    VirtIODevice *vdev,
    void *pages,
    void (*notify)(struct virtqueue *),
    void *control);

unsigned int vring_control_block_size();

#endif /* _VIRTIO_RING_ALLOCATION_H */
