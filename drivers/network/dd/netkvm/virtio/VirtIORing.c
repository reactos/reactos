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
#include "VirtIO.h"
#include "kdebugprint.h"
#include "virtio_ring.h"
#include "windows/virtio_ring_allocation.h"

#define DESC_INDEX(num, i) ((i) & ((num) - 1))

 /* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT	1
/* This marks a buffer as write-only (otherwise read-only). */
#define VIRTQ_DESC_F_WRITE	2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT	4

/* The Host uses this in used->flags to advise the Guest: don't kick me when
* you add a buffer.  It's unreliable, so it's simply an optimization.  Guest
* will still kick if it's out of buffers. */
#define VIRTQ_USED_F_NO_NOTIFY	1
/* The Guest uses this in avail->flags to advise the Host: don't interrupt me
* when you consume a buffer.  It's unreliable, so it's simply an
* optimization.  */
#define VIRTQ_AVAIL_F_NO_INTERRUPT	1

#pragma warning (push)
#pragma warning (disable:4200)

#include <pshpack1.h>

/* Virtio ring descriptors: 16 bytes.  These can chain together via "next". */
struct vring_desc {
    /* Address (guest-physical). */
    __virtio64 addr;
    /* Length. */
    __virtio32 len;
    /* The flags as indicated above. */
    __virtio16 flags;
    /* We chain unused descriptors via this, too */
    __virtio16 next;
};

struct vring_avail {
    __virtio16 flags;
    __virtio16 idx;
    __virtio16 ring[];
};

/* u32 is used here for ids for padding reasons. */
struct vring_used_elem {
    /* Index of start of used descriptor chain. */
    __virtio32 id;
    /* Total length of the descriptor chain which was used (written to) */
    __virtio32 len;
};

struct vring_used {
    __virtio16 flags;
    __virtio16 idx;
    struct vring_used_elem ring[];
};

#include <poppack.h>

/* Alignment requirements for vring elements.
* When using pre-virtio 1.0 layout, these fall out naturally.
*/
#define VRING_AVAIL_ALIGN_SIZE 2
#define VRING_USED_ALIGN_SIZE 4
#define VRING_DESC_ALIGN_SIZE 16

/* The standard layout for the ring is a continuous chunk of memory which looks
* like this.  We assume num is a power of 2.
*
* struct vring
* {
*	// The actual descriptors (16 bytes each)
*	struct vring_desc desc[num];
*
*	// A ring of available descriptor heads with free-running index.
*	__virtio16 avail_flags;
*	__virtio16 avail_idx;
*	__virtio16 available[num];
*	__virtio16 used_event_idx;
*
*	// Padding to the next align boundary.
*	char pad[];
*
*	// A ring of used descriptor heads with free-running index.
*	__virtio16 used_flags;
*	__virtio16 used_idx;
*	struct vring_used_elem used[num];
*	__virtio16 avail_event_idx;
* };
*/
/* We publish the used event index at the end of the available ring, and vice
* versa. They are at the end for backwards compatibility. */

struct vring {
    unsigned int num;

    struct vring_desc *desc;

    struct vring_avail *avail;

    struct vring_used *used;
};

#define vring_used_event(vr) ((vr)->avail->ring[(vr)->num])
#define vring_avail_event(vr) (*(__virtio16 *)&(vr)->used->ring[(vr)->num])

static inline void vring_init(struct vring *vr, unsigned int num, void *p,
    unsigned long align)
{
    vr->num = num;
    vr->desc = (struct vring_desc *)p;
    vr->avail = (struct vring_avail *)((__u8 *)p + num * sizeof(struct vring_desc));
    vr->used = (struct vring_used *)(((ULONG_PTR)&vr->avail->ring[num] + sizeof(__virtio16)
        + align - 1) & ~((ULONG_PTR)align - 1));
}

static inline unsigned vring_size_split(unsigned int num, unsigned long align)
{
#pragma warning (push)
#pragma warning (disable:4319)
    return ((sizeof(struct vring_desc) * num + sizeof(__virtio16) * (3 + num)
        + align - 1) & ~(align - 1))
        + sizeof(__virtio16) * 3 + sizeof(struct vring_used_elem) * num;
#pragma warning(pop)
}

/* The following is used with USED_EVENT_IDX and AVAIL_EVENT_IDX */
/* Assuming a given event_idx value from the other side, if
* we have just incremented index from old to new_idx,
* should we trigger an event? */
static inline int vring_need_event(__u16 event_idx, __u16 new_idx, __u16 old)
{
    /* Note: Xen has similar logic for notification hold-off
    * in include/xen/interface/io/ring.h with req_event and req_prod
    * corresponding to event_idx + 1 and new_idx respectively.
    * Note also that req_event and req_prod in Xen start at 1,
    * event indexes in virtio start at 0. */
    return (__u16)(new_idx - event_idx - 1) < (__u16)(new_idx - old);
}

struct virtqueue_split {
    struct virtqueue vq;
    struct vring vring;
    struct {
        u16 flags;
        u16 idx;
    } master_vring_avail;
    unsigned int num_unused;
    unsigned int num_added_since_kick;
    u16 first_unused;
    u16 last_used;
    void *opaque[];
};

#define splitvq(vq) ((struct virtqueue_split *)vq)

#pragma warning (pop)

 /* Returns the index of the first unused descriptor */
static inline u16 get_unused_desc(struct virtqueue_split *vq)
{
    u16 idx = vq->first_unused;
    ASSERT(vq->num_unused > 0);

    vq->first_unused = vq->vring.desc[idx].next;
    vq->num_unused--;
    return idx;
}

/* Marks the descriptor chain starting at index idx as unused */
static inline void put_unused_desc_chain(struct virtqueue_split *vq, u16 idx)
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
static int virtqueue_add_buf_split(
    struct virtqueue *_vq,    /* the queue */
    struct scatterlist sg[], /* sg array of length out + in */
    unsigned int out,        /* number of driver->device buffer descriptors in sg */
    unsigned int in,         /* number of device->driver buffer descriptors in sg */
    void *opaque,            /* later returned from virtqueue_get_buf */
    void *va_indirect,       /* VA of the indirect page or NULL */
    ULONGLONG phys_indirect) /* PA of the indirect page or 0 */
{
    struct virtqueue_split *vq = splitvq(_vq);
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
static void *virtqueue_get_buf_split(
    struct virtqueue *_vq, /* the queue */
    unsigned int *len)    /* number of bytes returned by the device */
{
    struct virtqueue_split *vq = splitvq(_vq);
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
    if (_vq->vdev->event_suppression_enabled && virtqueue_is_interrupt_enabled(_vq)) {
        vring_used_event(&vq->vring) = vq->last_used;
        KeMemoryBarrier();
    }

    ASSERT(opaque != NULL);
    return opaque;
}

/* Returns true if at least one returned buffer is available, false otherwise */
static BOOLEAN virtqueue_has_buf_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
    return (vq->last_used != vq->vring.used->idx);
}

/* Returns true if the device should be notified, false otherwise */
static bool virtqueue_kick_prepare_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
    bool wrap_around;
    u16 old, new;
    KeMemoryBarrier();

    wrap_around = (vq->num_added_since_kick >= (1 << 16));

    old = (u16)(vq->master_vring_avail.idx - vq->num_added_since_kick);
    new = vq->master_vring_avail.idx;
    vq->num_added_since_kick = 0;

    if (_vq->vdev->event_suppression_enabled) {
        return wrap_around || (bool)vring_need_event(vring_avail_event(&vq->vring), new, old);
    } else {
        return !(vq->vring.used->flags & VIRTQ_USED_F_NO_NOTIFY);
    }
}

/* Notifies the device even if it's not necessary according to the event suppression logic */
static void virtqueue_kick_always_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
    KeMemoryBarrier();
    vq->num_added_since_kick = 0;
    virtqueue_notify(_vq);
}

/* Enables interrupts on a virtqueue and returns false if the queue has at least one returned
 * buffer available to be fetched by virtqueue_get_buf, true otherwise */
static bool virtqueue_enable_cb_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
    if (!virtqueue_is_interrupt_enabled(_vq)) {
        vq->master_vring_avail.flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
        if (!_vq->vdev->event_suppression_enabled)
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
static bool virtqueue_enable_cb_delayed_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
    u16 bufs;

    if (!virtqueue_is_interrupt_enabled(_vq)) {
        vq->master_vring_avail.flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
        if (!_vq->vdev->event_suppression_enabled)
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
static void virtqueue_disable_cb_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
    if (virtqueue_is_interrupt_enabled(_vq)) {
        vq->master_vring_avail.flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;
        if (!_vq->vdev->event_suppression_enabled)
        {
            vq->vring.avail->flags = vq->master_vring_avail.flags;
        }
    }
}

/* Returns true if interrupts are enabled on a virtqueue, false otherwise */
static BOOLEAN virtqueue_is_interrupt_enabled_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
    return !(vq->master_vring_avail.flags & VIRTQ_AVAIL_F_NO_INTERRUPT);
}

/* Re-initializes an already initialized virtqueue */
static void virtqueue_shutdown_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
    unsigned int num = vq->vring.num;
    void *pages = vq->vring.desc;
    unsigned int vring_align = _vq->vdev->addr ? PAGE_SIZE : SMP_CACHE_BYTES;

    RtlZeroMemory(pages, vring_size_split(num, vring_align));
    (void)vring_new_virtqueue_split(
        _vq->index,
        vq->vring.num,
        vring_align,
        _vq->vdev,
        pages,
        _vq->notification_cb,
        vq);
}

/* Gets the opaque pointer associated with a not-yet-returned buffer, or NULL if no buffer is available
 * to aid drivers with cleaning up all data on virtqueue shutdown */
static void *virtqueue_detach_unused_buf_split(struct virtqueue *_vq)
{
    struct virtqueue_split *vq = splitvq(_vq);
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

/* Returns the size of the virtqueue structure including
 * additional size for per-descriptor data */
unsigned int vring_control_block_size(u16 qsize, bool packed)
{
    unsigned int res;
    if (packed) {
        return vring_control_block_size_packed(qsize);
    }
    res = sizeof(struct virtqueue_split);
    res += sizeof(void *) * qsize;
    return res;
}

/* Initializes a new virtqueue using already allocated memory */
struct virtqueue *vring_new_virtqueue_split(
    unsigned int index,                 /* virtqueue index */
    unsigned int num,                   /* virtqueue size (always a power of 2) */
    unsigned int vring_align,           /* vring alignment requirement */
    VirtIODevice *vdev,                 /* the virtio device owning the queue */
    void *pages,                        /* vring memory */
    void(*notify)(struct virtqueue *), /* notification callback */
    void *control)                      /* virtqueue memory */
{
    struct virtqueue_split *vq = splitvq(control);
    u16 i;

    if (DESC_INDEX(num, num) != 0) {
        DPrintf(0, "Virtqueue length %u is not a power of 2\n", num);
        return NULL;
    }

    RtlZeroMemory(vq, sizeof(*vq) + num * sizeof(void *));

    vring_init(&vq->vring, num, pages, vring_align);
    vq->vq.vdev = vdev;
    vq->vq.notification_cb = notify;
    vq->vq.index = index;

    /* Build a linked list of unused descriptors */
    vq->num_unused = num;
    vq->first_unused = 0;
    for (i = 0; i < num - 1; i++) {
        vq->vring.desc[i].flags = VIRTQ_DESC_F_NEXT;
        vq->vring.desc[i].next = i + 1;
    }
    vq->vq.avail_va = vq->vring.avail;
    vq->vq.used_va = vq->vring.used;
    vq->vq.add_buf = virtqueue_add_buf_split;
    vq->vq.detach_unused_buf = virtqueue_detach_unused_buf_split;
    vq->vq.disable_cb = virtqueue_disable_cb_split;
    vq->vq.enable_cb = virtqueue_enable_cb_split;
    vq->vq.enable_cb_delayed = virtqueue_enable_cb_delayed_split;
    vq->vq.get_buf = virtqueue_get_buf_split;
    vq->vq.has_buf = virtqueue_has_buf_split;
    vq->vq.is_interrupt_enabled = virtqueue_is_interrupt_enabled_split;
    vq->vq.kick_always = virtqueue_kick_always_split;
    vq->vq.kick_prepare = virtqueue_kick_prepare_split;
    vq->vq.shutdown = virtqueue_shutdown_split;
    return &vq->vq;
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

unsigned long vring_size(unsigned int num, unsigned long align, bool packed)
{
    if (packed) {
        return vring_size_packed(num, align);
    } else {
        return vring_size_split(num, align);
    }
}
