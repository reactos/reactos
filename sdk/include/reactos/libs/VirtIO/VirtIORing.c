/*
 * Virtio ring manipulation routines
 *
 * Copyright 2017 Red Hat, Inc.
 *
 * Authors:
 *  Ladi Prosek <lprosek@redhat.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "osdep.h"
#include "virtio_pci.h"
#include "virtio.h"
#include "kdebugprint.h"
#include "virtio_ring.h"

#define DESC_INDEX(num, i) ((i) & ((num) - 1))

/* Returns the index of the first unused descriptor */
static inline u16 get_unused_desc(struct virtqueue *vq)
{
    u16 idx = vq->first_unused;
    ASSERT(vq->num_unused > 0);

    vq->first_unused = vq->vring.desc[idx].next;
    vq->num_unused--;
    return idx;
}

/* Marks the descriptor chain starting at index idx as unused */
static inline void put_unused_desc_chain(struct virtqueue *vq, u16 idx)
{
    u16 start = idx;

    vq->opaque[idx] = NULL;
    while (vq->vring.desc[idx].flags & VIRTQ_DESC_F_NEXT) {
        idx = vq->vring.desc[idx].next;
        vq->num_unused++;
    }

    vq->vring.desc[idx].flags = VIRTQ_DESC_F_NEXT;
    vq->vring.desc[idx].next = vq->first_unused;
    vq->num_unused++;

    vq->first_unused = start;
}

/* Adds a buffer to a virtqueue, returns 0 on success, negative number on error */
int virtqueue_add_buf(
    struct virtqueue *vq,    /* the queue */
    struct scatterlist sg[], /* sg array of length out + in */
    unsigned int out,        /* number of driver->device buffer descriptors in sg */
    unsigned int in,         /* number of device->driver buffer descriptors in sg */
    void *opaque,            /* later returned from virtqueue_get_buf */
    void *va_indirect,       /* VA of the indirect page or NULL */
    ULONGLONG phys_indirect) /* PA of the indirect page or 0 */
{
    struct vring *vring = &vq->vring;
    unsigned int i;
    u16 idx;

    if (va_indirect && (out + in) > 1 && vq->num_unused > 0) {
        /* Use one indirect descriptor */
        struct vring_desc *desc = (struct vring_desc *)va_indirect;

        for (i = 0; i < out + in; i++) {
            desc[i].flags = (i < out ? 0 : VIRTQ_DESC_F_WRITE);
            desc[i].flags |= VIRTQ_DESC_F_NEXT;
            desc[i].addr = sg[i].physAddr.QuadPart;
            desc[i].len = sg[i].length;
            desc[i].next = (u16)i + 1;
        }
        desc[i - 1].flags &= ~VIRTQ_DESC_F_NEXT;

        idx = get_unused_desc(vq);
        vq->vring.desc[idx].flags = VIRTQ_DESC_F_INDIRECT;
        vq->vring.desc[idx].addr = phys_indirect;
        vq->vring.desc[idx].len = i * sizeof(struct vring_desc);

        vq->opaque[idx] = opaque;
    } else {
        u16 last_idx;

        /* Use out + in regular descriptors */
        if (out + in > vq->num_unused) {
            return -ENOSPC;
        }

        /* First descriptor */
        idx = last_idx = get_unused_desc(vq);
        vq->opaque[idx] = opaque;

        vring->desc[idx].addr = sg[0].physAddr.QuadPart;
        vring->desc[idx].len = sg[0].length;
        vring->desc[idx].flags = VIRTQ_DESC_F_NEXT;
        if (out == 0) {
            vring->desc[idx].flags |= VIRTQ_DESC_F_WRITE;
        }
        vring->desc[idx].next = vq->first_unused;

        /* The rest of descriptors */
        for (i = 1; i < out + in; i++) {
            last_idx = get_unused_desc(vq);

            vring->desc[last_idx].addr = sg[i].physAddr.QuadPart;
            vring->desc[last_idx].len = sg[i].length;
            vring->desc[last_idx].flags = VIRTQ_DESC_F_NEXT;
            if (i >= out) {
                vring->desc[last_idx].flags |= VIRTQ_DESC_F_WRITE;
            }
            vring->desc[last_idx].next = vq->first_unused;
        }
        vring->desc[last_idx].flags &= ~VIRTQ_DESC_F_NEXT;
    }

    /* Write the first descriptor into the available ring */
    vring->avail->ring[DESC_INDEX(vring->num, vq->master_vring_avail.idx)] = idx;
    KeMemoryBarrier();
    vring->avail->idx = ++vq->master_vring_avail.idx;
    vq->num_added_since_kick++;

    return 0;
}

/* Gets the opaque pointer associated with a returned buffer, or NULL if no buffer is available */
void *virtqueue_get_buf(
    struct virtqueue *vq, /* the queue */
    unsigned int *len)    /* number of bytes returned by the device */
{
    void *opaque;
    u16 idx;

    if (vq->last_used == (int)vq->vring.used->idx) {
        /* No descriptor index in the used ring */
        return NULL;
    }
    KeMemoryBarrier();

    idx = DESC_INDEX(vq->vring.num, vq->last_used);
    *len = vq->vring.used->ring[idx].len;

    /* Get the first used descriptor */
    idx = (u16)vq->vring.used->ring[idx].id;
    opaque = vq->opaque[idx];

    /* Put all descriptors back to the free list */
    put_unused_desc_chain(vq, idx);

    vq->last_used++;
    if (vq->vdev->event_suppression_enabled && virtqueue_is_interrupt_enabled(vq)) {
        vring_used_event(&vq->vring) = vq->last_used;
        KeMemoryBarrier();
    }

    ASSERT(opaque != NULL);
    return opaque;
}

/* Returns true if at least one returned buffer is available, false otherwise */
BOOLEAN virtqueue_has_buf(struct virtqueue *vq)
{
    return (vq->last_used != vq->vring.used->idx);
}

/* Returns true if the device should be notified, false otherwise */
bool virtqueue_kick_prepare(struct virtqueue *vq)
{
    bool wrap_around;
    u16 old, new;
    KeMemoryBarrier();

    wrap_around = (vq->num_added_since_kick >= (1 << 16));

    old = (u16)(vq->master_vring_avail.idx - vq->num_added_since_kick);
    new = vq->master_vring_avail.idx;
    vq->num_added_since_kick = 0;

    if (vq->vdev->event_suppression_enabled) {
        return wrap_around || (bool)vring_need_event(vring_avail_event(&vq->vring), new, old);
    } else {
        return !(vq->vring.used->flags & VIRTQ_USED_F_NO_NOTIFY);
    }
}

/* Notifies the device even if it's not necessary according to the event suppression logic */
void virtqueue_kick_always(struct virtqueue *vq)
{
    KeMemoryBarrier();
    vq->num_added_since_kick = 0;
    virtqueue_notify(vq);
}

/* Enables interrupts on a virtqueue and returns false if the queue has at least one returned
 * buffer available to be fetched by virtqueue_get_buf, true otherwise */
bool virtqueue_enable_cb(struct virtqueue *vq)
{
    if (!virtqueue_is_interrupt_enabled(vq)) {
        vq->master_vring_avail.flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
        if (!vq->vdev->event_suppression_enabled)
        {
            vq->vring.avail->flags = vq->master_vring_avail.flags;
        }
    }

    vring_used_event(&vq->vring) = vq->last_used;
    KeMemoryBarrier();
    return (vq->last_used == vq->vring.used->idx);
}

/* Enables interrupts on a virtqueue after ~3/4 of the currently pushed buffers have been
 * returned, returns false if this condition currently holds, false otherwise */
bool virtqueue_enable_cb_delayed(struct virtqueue *vq)
{
    u16 bufs;

    if (!virtqueue_is_interrupt_enabled(vq)) {
        vq->master_vring_avail.flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
        if (!vq->vdev->event_suppression_enabled)
        {
            vq->vring.avail->flags = vq->master_vring_avail.flags;
        }
    }

    /* Note that 3/4 is an arbitrary threshold */
    bufs = (u16)(vq->master_vring_avail.idx - vq->last_used) * 3 / 4;
    vring_used_event(&vq->vring) = vq->last_used + bufs;
    KeMemoryBarrier();
    return ((vq->vring.used->idx - vq->last_used) <= bufs);
}

/* Disables interrupts on a virtqueue */
void virtqueue_disable_cb(struct virtqueue *vq)
{
    if (virtqueue_is_interrupt_enabled(vq)) {
        vq->master_vring_avail.flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;
        if (!vq->vdev->event_suppression_enabled)
        {
            vq->vring.avail->flags = vq->master_vring_avail.flags;
        }
    }
}

/* Returns true if interrupts are enabled on a virtqueue, false otherwise */
BOOLEAN virtqueue_is_interrupt_enabled(struct virtqueue *vq)
{
    return !(vq->master_vring_avail.flags & VIRTQ_AVAIL_F_NO_INTERRUPT);
}

/* Initializes a new virtqueue using already allocated memory */
struct virtqueue *vring_new_virtqueue(
    unsigned int index,                 /* virtqueue index */
    unsigned int num,                   /* virtqueue size (always a power of 2) */
    unsigned int vring_align,           /* vring alignment requirement */
    VirtIODevice *vdev,                 /* the virtio device owning the queue */
    void *pages,                        /* vring memory */
    void (*notify)(struct virtqueue *), /* notification callback */
    void *control)                      /* virtqueue memory */
{
    struct virtqueue *vq = (struct virtqueue *)control;
    u16 i;

    if (DESC_INDEX(num, num) != 0) {
        DPrintf(0, "Virtqueue length %u is not a power of 2\n", num);
        return NULL;
    }

    RtlZeroMemory(vq, sizeof(*vq) + num * sizeof(void *));

    vring_init(&vq->vring, num, pages, vring_align);
    vq->vdev = vdev;
    vq->notification_cb = notify;
    vq->index = index;

    /* Build a linked list of unused descriptors */
    vq->num_unused = num;
    vq->first_unused = 0;
    for (i = 0; i < num - 1; i++) {
        vq->vring.desc[i].flags = VIRTQ_DESC_F_NEXT;
        vq->vring.desc[i].next = i + 1;
    }
    return vq;
}

/* Re-initializes an already initialized virtqueue */
void virtqueue_shutdown(struct virtqueue *vq)
{
    unsigned int num = vq->vring.num;
    void *pages = vq->vring.desc;
    unsigned int vring_align = vq->vdev->addr ? PAGE_SIZE : SMP_CACHE_BYTES;

    RtlZeroMemory(pages, vring_size(num, vring_align));
    (void)vring_new_virtqueue(
        vq->index,
        vq->vring.num,
        vring_align,
        vq->vdev,
        pages,
        vq->notification_cb,
        vq);
}

/* Gets the opaque pointer associated with a not-yet-returned buffer, or NULL if no buffer is available
 * to aid drivers with cleaning up all data on virtqueue shutdown */
void *virtqueue_detach_unused_buf(struct virtqueue *vq)
{
    u16 idx;
    void *opaque = NULL;

    for (idx = 0; idx < (u16)vq->vring.num; idx++) {
        opaque = vq->opaque[idx];
        if (opaque) {
            put_unused_desc_chain(vq, idx);
            vq->vring.avail->idx = --vq->master_vring_avail.idx;
            break;
        }
    }
    return opaque;
}

/* Returns the base size of the virtqueue structure (add sizeof(void *) times the number
 * of elements to get the the full size) */
unsigned int vring_control_block_size()
{
    return sizeof(struct virtqueue);
}

/* Negotiates virtio transport features */
void vring_transport_features(
    VirtIODevice *vdev,
    u64 *features) /* points to device features on entry and driver accepted features on return */
{
    unsigned int i;

    for (i = VIRTIO_TRANSPORT_F_START; i < VIRTIO_TRANSPORT_F_END; i++) {
        if (i != VIRTIO_RING_F_INDIRECT_DESC &&
            i != VIRTIO_RING_F_EVENT_IDX &&
            i != VIRTIO_F_VERSION_1) {
            virtio_feature_disable(*features, i);
        }
    }
}

/* Returns the max number of scatter-gather elements that fit in an indirect pages */
u32 virtio_get_indirect_page_capacity()
{
    return PAGE_SIZE / sizeof(struct vring_desc);
}
