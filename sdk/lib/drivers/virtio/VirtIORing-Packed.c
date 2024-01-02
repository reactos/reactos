/*
 * Packed virtio ring manipulation routines
 *
 * Copyright 2019 Red Hat, Inc.
 *
 * Authors:
 *  Yuri Benditovich <ybendito@redhat.com>
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

#include <pshpack1.h>

struct vring_packed_desc_event {
    /* Descriptor Ring Change Event Offset/Wrap Counter. */
    __le16 off_wrap;
    /* Descriptor Ring Change Event Flags. */
    __le16 flags;
};

struct vring_packed_desc {
    /* Buffer Address. */
    __virtio64 addr;
    /* Buffer Length. */
    __le32 len;
    /* Buffer ID. */
    __le16 id;
    /* The flags depending on descriptor type. */
    __le16 flags;
};

#include <poppack.h>

#define BUG_ON(condition) { if (condition) { KeBugCheck(0xE0E1E2E3); }}
#define BAD_RING(vq, fmt, ...) DPrintf(0, "%s: queue %d: " fmt, __FUNCTION__, vq->vq.index, __VA_ARGS__); BUG_ON(true)

/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT	1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE	2
/* This means the buffer contains a list of buffer descriptors. */
#define VRING_DESC_F_INDIRECT	4

/*
 * Mark a descriptor as available or used in packed ring.
 * Notice: they are defined as shifts instead of shifted values.
 */
#define VRING_PACKED_DESC_F_AVAIL	7
#define VRING_PACKED_DESC_F_USED	15

/* Enable events in packed ring. */
#define VRING_PACKED_EVENT_FLAG_ENABLE	0x0
/* Disable events in packed ring. */
#define VRING_PACKED_EVENT_FLAG_DISABLE	0x1

/*
 * Enable events for a specific descriptor in packed ring.
 * (as specified by Descriptor Ring Change Event Offset/Wrap Counter).
 * Only valid if VIRTIO_RING_F_EVENT_IDX has been negotiated.
 */
#define VRING_PACKED_EVENT_FLAG_DESC	0x2
 /*
  * Wrap counter bit shift in event suppression structure
  * of packed ring.
  */
#define VRING_PACKED_EVENT_F_WRAP_CTR	15

/* The following is used with USED_EVENT_IDX and AVAIL_EVENT_IDX */
/* Assuming a given event_idx value from the other side, if
 * we have just incremented index from old to new_idx,
 * should we trigger an event?
 */
static inline bool vring_need_event(__u16 event_idx, __u16 new_idx, __u16 old)
{
    /* Note: Xen has similar logic for notification hold-off
    * in include/xen/interface/io/ring.h with req_event and req_prod
    * corresponding to event_idx + 1 and new_idx respectively.
    * Note also that req_event and req_prod in Xen start at 1,
    * event indexes in virtio start at 0. */
    return (__u16)(new_idx - event_idx - 1) < (__u16)(new_idx - old);
}

struct vring_desc_state_packed {
    void *data;			/* Data for callback. */
    u16 num;			/* Descriptor list length. */
    u16 next;			/* The next desc state in a list. */
    u16 last;			/* The last desc state in a list. */
};

struct virtqueue_packed {
    struct virtqueue vq;
    /* Number we've added since last sync. */
    unsigned int num_added;
    /* Head of free buffer list. */
    unsigned int free_head;
    /* Number of free descriptors */
    unsigned int num_free;
    /* Last used index we've seen. */
    u16 last_used_idx;
    /* Avail used flags. */
    u16 avail_used_flags;
    struct
    {
        /* Driver ring wrap counter. */
        bool avail_wrap_counter;
        /* Device ring wrap counter. */
        bool used_wrap_counter;
        /* Index of the next avail descriptor. */
        u16 next_avail_idx;
        /*
         * Last written value to driver->flags in
         * guest byte order.
         */
        u16 event_flags_shadow;
        struct {
            unsigned int num;
            struct vring_packed_desc *desc;
            struct vring_packed_desc_event *driver;
            struct vring_packed_desc_event *device;
        } vring;
        /* Per-descriptor state. */
        struct vring_desc_state_packed *desc_state;
    } packed;
    struct vring_desc_state_packed desc_states[];
};

#define packedvq(vq) ((struct virtqueue_packed *)vq)

unsigned int vring_control_block_size_packed(u16 qsize)
{
    return sizeof(struct virtqueue_packed) + sizeof(struct vring_desc_state_packed) * qsize;
}

unsigned long vring_size_packed(unsigned int num, unsigned long align)
{
    /* array of descriptors */
    unsigned long res = num * sizeof(struct vring_packed_desc);
    /* driver and device event */
    res += 2 * sizeof(struct vring_packed_desc_event);
    return res;
}

static int virtqueue_add_buf_packed(
    struct virtqueue *_vq,    /* the queue */
    struct scatterlist sg[], /* sg array of length out + in */
    unsigned int out,        /* number of driver->device buffer descriptors in sg */
    unsigned int in,         /* number of device->driver buffer descriptors in sg */
    void *opaque,            /* later returned from virtqueue_get_buf */
    void *va_indirect,       /* VA of the indirect page or NULL */
    ULONGLONG phys_indirect) /* PA of the indirect page or 0 */
{
    struct virtqueue_packed *vq = packedvq(_vq);
    unsigned int descs_used;
    struct vring_packed_desc *desc;
    u16 head, id, i;

    descs_used = out + in;
    head = vq->packed.next_avail_idx;
    id = (u16)vq->free_head;

    BUG_ON(descs_used == 0);
    BUG_ON(id >= vq->packed.vring.num);

    if (va_indirect && vq->num_free > 0) {
        desc = va_indirect;
        for (i = 0; i < descs_used; i++) {
            desc[i].flags = i < out ? 0 : VRING_DESC_F_WRITE;
            desc[i].addr = sg[i].physAddr.QuadPart;
            desc[i].len = sg[i].length;
        }
        vq->packed.vring.desc[head].addr = phys_indirect;
        vq->packed.vring.desc[head].len = descs_used * sizeof(struct vring_packed_desc);
        vq->packed.vring.desc[head].id = id;

        KeMemoryBarrier();
        vq->packed.vring.desc[head].flags = VRING_DESC_F_INDIRECT | vq->avail_used_flags;

        DPrintf(5, "Added buffer head %i to Q%d\n", head, vq->vq.index);
        head++;
        if (head >= vq->packed.vring.num) {
            head = 0;
            vq->packed.avail_wrap_counter ^= 1;
            vq->avail_used_flags ^=
                1 << VRING_PACKED_DESC_F_AVAIL |
                1 << VRING_PACKED_DESC_F_USED;
        }
        vq->packed.next_avail_idx = head;
        /* We're using some buffers from the free list. */
        vq->num_free -= 1;
        vq->num_added += 1;

        vq->free_head = vq->packed.desc_state[id].next;

        /* Store token and indirect buffer state. */
        vq->packed.desc_state[id].num = 1;
        vq->packed.desc_state[id].data = opaque;
        vq->packed.desc_state[id].last = id;

    } else {
        unsigned int n;
        u16 curr, prev, head_flags;
        if (vq->num_free < descs_used) {
            DPrintf(6, "Can't add buffer to Q%d\n", vq->vq.index);
            return -ENOSPC;
        }
        desc = vq->packed.vring.desc;
        i = head;
        curr = id;
        for (n = 0; n < descs_used; n++) {
            u16 flags = vq->avail_used_flags;
            flags |= n < out ? 0 : VRING_DESC_F_WRITE;
            if (n != descs_used - 1) {
                flags |= VRING_DESC_F_NEXT;
            }
            desc[i].addr = sg[n].physAddr.QuadPart;
            desc[i].len = sg[n].length;
            desc[i].id = id;
            if (n == 0) {
                head_flags = flags;
            }
            else {
                desc[i].flags = flags;
            }

            prev = curr;
            curr = vq->packed.desc_state[curr].next;

            if (++i >= vq->packed.vring.num) {
                i = 0;
                vq->avail_used_flags ^=
                    1 << VRING_PACKED_DESC_F_AVAIL |
                    1 << VRING_PACKED_DESC_F_USED;
            }
        }

        if (i < head)
            vq->packed.avail_wrap_counter ^= 1;

        /* We're using some buffers from the free list. */
        vq->num_free -= descs_used;

        /* Update free pointer */
        vq->packed.next_avail_idx = i;
        vq->free_head = curr;

        /* Store token. */
        vq->packed.desc_state[id].num = (u16)descs_used;
        vq->packed.desc_state[id].data = opaque;
        vq->packed.desc_state[id].last = prev;

        /*
         * A driver MUST NOT make the first descriptor in the list
         * available before all subsequent descriptors comprising
         * the list are made available.
         */
        KeMemoryBarrier();
        vq->packed.vring.desc[head].flags = head_flags;
        vq->num_added += descs_used;

        DPrintf(5, "Added buffer head @%i+%d to Q%d\n", head, descs_used, vq->vq.index);
    }

    return 0;
}

static void detach_buf_packed(struct virtqueue_packed *vq, unsigned int id)
{
    struct vring_desc_state_packed *state = &vq->packed.desc_state[id];

    /* Clear data ptr. */
    state->data = NULL;

    vq->packed.desc_state[state->last].next = (u16)vq->free_head;
    vq->free_head = id;
    vq->num_free += state->num;
}

static void *virtqueue_detach_unused_buf_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);
    unsigned int i;
    void *buf;

    for (i = 0; i < vq->packed.vring.num; i++) {
        if (!vq->packed.desc_state[i].data)
            continue;
        /* detach_buf clears data, so grab it now. */
        buf = vq->packed.desc_state[i].data;
        detach_buf_packed(vq, i);
        return buf;
    }
    /* That should have freed everything. */
    BUG_ON(vq->num_free != vq->packed.vring.num);

    return NULL;
}

static void virtqueue_disable_cb_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);

    if (vq->packed.event_flags_shadow != VRING_PACKED_EVENT_FLAG_DISABLE) {
        vq->packed.event_flags_shadow = VRING_PACKED_EVENT_FLAG_DISABLE;
        vq->packed.vring.driver->flags = vq->packed.event_flags_shadow;
    }
}

static inline bool is_used_desc_packed(const struct virtqueue_packed *vq,
    u16 idx, bool used_wrap_counter)
{
    bool avail, used;
    u16 flags;

    flags = vq->packed.vring.desc[idx].flags;
    avail = !!(flags & (1 << VRING_PACKED_DESC_F_AVAIL));
    used = !!(flags & (1 << VRING_PACKED_DESC_F_USED));

    return avail == used && used == used_wrap_counter;
}

static inline bool virtqueue_poll_packed(struct virtqueue_packed *vq, u16 off_wrap)
{
    bool wrap_counter;
    u16 used_idx;
    KeMemoryBarrier();

    wrap_counter = off_wrap >> VRING_PACKED_EVENT_F_WRAP_CTR;
    used_idx = off_wrap & ~(1 << VRING_PACKED_EVENT_F_WRAP_CTR);

    return is_used_desc_packed(vq, used_idx, wrap_counter);

}

static inline unsigned virtqueue_enable_cb_prepare_packed(struct virtqueue_packed *vq)
{
    bool event_suppression_enabled = vq->vq.vdev->event_suppression_enabled;
    /*
     * We optimistically turn back on interrupts, then check if there was
     * more to do.
     */

    if (event_suppression_enabled) {
        vq->packed.vring.driver->off_wrap =
            vq->last_used_idx |
            (vq->packed.used_wrap_counter <<
                VRING_PACKED_EVENT_F_WRAP_CTR);
        /*
         * We need to update event offset and event wrap
         * counter first before updating event flags.
         */
        KeMemoryBarrier();
    }

    if (vq->packed.event_flags_shadow == VRING_PACKED_EVENT_FLAG_DISABLE) {
        vq->packed.event_flags_shadow = event_suppression_enabled ?
            VRING_PACKED_EVENT_FLAG_DESC :
            VRING_PACKED_EVENT_FLAG_ENABLE;
        vq->packed.vring.driver->flags = vq->packed.event_flags_shadow;
    }

    return vq->last_used_idx | ((u16)vq->packed.used_wrap_counter <<
        VRING_PACKED_EVENT_F_WRAP_CTR);
}

static bool virtqueue_enable_cb_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);
    unsigned last_used_idx = virtqueue_enable_cb_prepare_packed(vq);

    return !virtqueue_poll_packed(vq, (u16)last_used_idx);
}

static bool virtqueue_enable_cb_delayed_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);
    bool event_suppression_enabled = vq->vq.vdev->event_suppression_enabled;
    u16 used_idx, wrap_counter;
    u16 bufs;

    /*
     * We optimistically turn back on interrupts, then check if there was
     * more to do.
     */

    if (event_suppression_enabled) {
        /* TODO: tune this threshold */
        bufs = (vq->packed.vring.num - vq->num_free) * 3 / 4;
        wrap_counter = vq->packed.used_wrap_counter;

        used_idx = vq->last_used_idx + bufs;
        if (used_idx >= vq->packed.vring.num) {
            used_idx -= (u16)vq->packed.vring.num;
            wrap_counter ^= 1;
        }

        vq->packed.vring.driver->off_wrap = used_idx |
            (wrap_counter << VRING_PACKED_EVENT_F_WRAP_CTR);

        /*
         * We need to update event offset and event wrap
         * counter first before updating event flags.
         */
        KeMemoryBarrier();
    }

    if (vq->packed.event_flags_shadow == VRING_PACKED_EVENT_FLAG_DISABLE) {
        vq->packed.event_flags_shadow = event_suppression_enabled ?
            VRING_PACKED_EVENT_FLAG_DESC :
            VRING_PACKED_EVENT_FLAG_ENABLE;
        vq->packed.vring.driver->flags = vq->packed.event_flags_shadow;
    }

    /*
     * We need to update event suppression structure first
     * before re-checking for more used buffers.
     */
    KeMemoryBarrier();

    if (is_used_desc_packed(vq,
        vq->last_used_idx,
        vq->packed.used_wrap_counter)) {
        return false;
    }

    return true;
}

static BOOLEAN virtqueue_is_interrupt_enabled_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);
    return vq->packed.event_flags_shadow & VRING_PACKED_EVENT_FLAG_DISABLE;
}

static void virtqueue_shutdown_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);
    unsigned int num = vq->packed.vring.num;
    void *pages = vq->packed.vring.desc;
    unsigned int vring_align = _vq->vdev->addr ? PAGE_SIZE : SMP_CACHE_BYTES;

    RtlZeroMemory(pages, vring_size_packed(num, vring_align));
    vring_new_virtqueue_packed(
        _vq->index,
        num,
        vring_align,
        _vq->vdev,
        pages,
        _vq->notification_cb,
        _vq);
}

static inline bool more_used_packed(const struct virtqueue_packed *vq)
{
    return is_used_desc_packed(vq, vq->last_used_idx,
        vq->packed.used_wrap_counter);
}

static void *virtqueue_get_buf_packed(
    struct virtqueue *_vq, /* the queue */
    unsigned int *len)    /* number of bytes returned by the device */
{
    struct virtqueue_packed *vq = packedvq(_vq);
    u16 last_used, id;
    void *ret;

    if (!more_used_packed(vq)) {
        DPrintf(6, "%s: No more buffers in queue\n", __FUNCTION__);
        return NULL;
    }

    /* Only get used elements after they have been exposed by host. */
    KeMemoryBarrier();

    last_used = vq->last_used_idx;
    id = vq->packed.vring.desc[last_used].id;
    *len = vq->packed.vring.desc[last_used].len;

    if (id >= vq->packed.vring.num) {
        BAD_RING(vq, "id %u out of range\n", id);
        return NULL;
    }
    if (!vq->packed.desc_state[id].data) {
        BAD_RING(vq, "id %u is not a head!\n", id);
        return NULL;
    }

    /* detach_buf_packed clears data, so grab it now. */
    ret = vq->packed.desc_state[id].data;
    detach_buf_packed(vq, id);

    vq->last_used_idx += vq->packed.desc_state[id].num;
    if (vq->last_used_idx >= vq->packed.vring.num) {
        vq->last_used_idx -= (u16)vq->packed.vring.num;
        vq->packed.used_wrap_counter ^= 1;
    }

    /*
     * If we expect an interrupt for the next entry, tell host
     * by writing event index and flush out the write before
     * the read in the next get_buf call.
     */
    if (vq->packed.event_flags_shadow == VRING_PACKED_EVENT_FLAG_DESC) {
        vq->packed.vring.driver->off_wrap = vq->last_used_idx |
            ((u16)vq->packed.used_wrap_counter <<
                VRING_PACKED_EVENT_F_WRAP_CTR);
        KeMemoryBarrier();
    }

    return ret;
}

static BOOLEAN virtqueue_has_buf_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);
    return more_used_packed(vq);
}

static bool virtqueue_kick_prepare_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);
    u16 new, old, off_wrap, flags, wrap_counter, event_idx;
    bool needs_kick;
    union {
        struct {
            __le16 off_wrap;
            __le16 flags;
        };
        u32 value32;
    } snapshot;

    /*
     * We need to expose the new flags value before checking notification
     * suppressions.
     */
    KeMemoryBarrier();

    old = vq->packed.next_avail_idx - vq->num_added;
    new = vq->packed.next_avail_idx;
    vq->num_added = 0;

    snapshot.value32 = *(u32 *)vq->packed.vring.device;
    flags = snapshot.flags;

    if (flags != VRING_PACKED_EVENT_FLAG_DESC) {
        needs_kick = (flags != VRING_PACKED_EVENT_FLAG_DISABLE);
        goto out;
    }

    off_wrap = snapshot.off_wrap;

    wrap_counter = off_wrap >> VRING_PACKED_EVENT_F_WRAP_CTR;
    event_idx = off_wrap & ~(1 << VRING_PACKED_EVENT_F_WRAP_CTR);
    if (wrap_counter != vq->packed.avail_wrap_counter)
        event_idx -= (u16)vq->packed.vring.num;

    needs_kick = vring_need_event(event_idx, new, old);
out:
    return needs_kick;
}

static void virtqueue_kick_always_packed(struct virtqueue *_vq)
{
    struct virtqueue_packed *vq = packedvq(_vq);
    KeMemoryBarrier();
    vq->num_added = 0;
    virtqueue_notify(_vq);
}

/* Initializes a new virtqueue using already allocated memory */
struct virtqueue *vring_new_virtqueue_packed(
    unsigned int index,                 /* virtqueue index */
    unsigned int num,                   /* virtqueue size (always a power of 2) */
    unsigned int vring_align,           /* vring alignment requirement */
    VirtIODevice *vdev,                 /* the virtio device owning the queue */
    void *pages,                        /* vring memory */
    void(*notify)(struct virtqueue *), /* notification callback */
    void *control)                      /* virtqueue memory */
{
    struct virtqueue_packed *vq = packedvq(control);
    unsigned int i;

    vq->vq.vdev = vdev;
    vq->vq.notification_cb = notify;
    vq->vq.index = index;

    vq->vq.avail_va = (u8 *)pages + num * sizeof(struct vring_packed_desc);
    vq->vq.used_va = (u8 *)vq->vq.avail_va + sizeof(struct vring_packed_desc_event);

    /* initialize the ring */
    vq->packed.vring.num = num;
    vq->packed.vring.desc = pages;
    vq->packed.vring.driver = vq->vq.avail_va;
    vq->packed.vring.device = vq->vq.used_va;

    vq->num_free = num;
    vq->free_head = 0;
    vq->num_added = 0;
    vq->packed.avail_wrap_counter = 1;
    vq->packed.used_wrap_counter = 1;
    vq->last_used_idx = 0;
    vq->avail_used_flags = 1 << VRING_PACKED_DESC_F_AVAIL;
    vq->packed.next_avail_idx = 0;
    vq->packed.event_flags_shadow = 0;
    vq->packed.desc_state = vq->desc_states;

    RtlZeroMemory(vq->packed.desc_state, num * sizeof(*vq->packed.desc_state));
    for (i = 0; i < num - 1; i++) {
        vq->packed.desc_state[i].next = i + 1;
    }

    vq->vq.add_buf = virtqueue_add_buf_packed;
    vq->vq.detach_unused_buf = virtqueue_detach_unused_buf_packed;
    vq->vq.disable_cb = virtqueue_disable_cb_packed;
    vq->vq.enable_cb = virtqueue_enable_cb_packed;
    vq->vq.enable_cb_delayed = virtqueue_enable_cb_delayed_packed;
    vq->vq.get_buf = virtqueue_get_buf_packed;
    vq->vq.has_buf = virtqueue_has_buf_packed;
    vq->vq.is_interrupt_enabled = virtqueue_is_interrupt_enabled_packed;
    vq->vq.kick_always = virtqueue_kick_always_packed;
    vq->vq.kick_prepare = virtqueue_kick_prepare_packed;
    vq->vq.shutdown = virtqueue_shutdown_packed;
    return &vq->vq;
}
