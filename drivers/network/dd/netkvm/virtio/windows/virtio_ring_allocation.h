#ifndef _VIRTIO_RING_ALLOCATION_H
#define _VIRTIO_RING_ALLOCATION_H

struct virtqueue *vring_new_virtqueue_split(unsigned int index,
    unsigned int num,
    unsigned int vring_align,
    VirtIODevice *vdev,
    void *pages,
    void (*notify)(struct virtqueue *),
    void *control);

struct virtqueue *vring_new_virtqueue_packed(unsigned int index,
    unsigned int num,
    unsigned int vring_align,
    VirtIODevice *vdev,
    void *pages,
    void (*notify)(struct virtqueue *),
    void *control);

unsigned int vring_control_block_size(u16 qsize, bool packed);
unsigned int vring_control_block_size_packed(u16 qsize);
unsigned long vring_size_packed(unsigned int num, unsigned long align);

#endif /* _VIRTIO_RING_ALLOCATION_H */
