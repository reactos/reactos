/*
 * Virtio PCI driver - modern (virtio 1.0) device support
 *
 * Copyright IBM Corp. 2007
 * Copyright Red Hat, Inc. 2014
 *
 * Authors:
 *  Anthony Liguori  <aliguori@us.ibm.com>
 *  Rusty Russell <rusty@rustcorp.com.au>
 *  Michael S. Tsirkin <mst@redhat.com>
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
#define VIRTIO_PCI_NO_LEGACY
#include "virtio_pci.h"
#include "VirtIO.h"
#include "kdebugprint.h"
#include "virtio_ring.h"
#include "virtio_pci_common.h"
#include "windows/virtio_ring_allocation.h"
#include <stddef.h>

#ifdef WPP_EVENT_TRACING
#include "VirtIOPCIModern.tmh"
#endif

static void *vio_modern_map_capability(VirtIODevice *vdev, int cap_offset,
                                       size_t minlen, u32 alignment,
                                       u32 start, u32 size, size_t *len)
{
    u8 bar;
    u32 bar_offset, bar_length;
    void *addr;

    pci_read_config_byte(vdev, cap_offset + offsetof(struct virtio_pci_cap, bar), &bar);
    pci_read_config_dword(vdev, cap_offset + offsetof(struct virtio_pci_cap, offset), &bar_offset);
    pci_read_config_dword(vdev, cap_offset + offsetof(struct virtio_pci_cap, length), &bar_length);

    if (start + minlen > bar_length) {
        DPrintf(0, "bar %i cap is not large enough to map %zu bytes at offset %u\n", bar, minlen, start);
        return NULL;
    }

    bar_length -= start;
    bar_offset += start;

    if (bar_offset & (alignment - 1)) {
        DPrintf(0, "bar %i offset %u not aligned to %u\n", bar, bar_offset, alignment);
        return NULL;
    }

    if (bar_length > size) {
        bar_length = size;
    }

    if (len) {
        *len = bar_length;
    }

    if (bar_offset + minlen > pci_get_resource_len(vdev, bar)) {
        DPrintf(0, "bar %i is not large enough to map %zu bytes at offset %u\n", bar, minlen, bar_offset);
        return NULL;
    }

    addr = pci_map_address_range(vdev, bar, bar_offset, bar_length);
    if (!addr) {
        DPrintf(0, "unable to map %u bytes at bar %i offset %u\n", bar_length, bar, bar_offset);
    }
    return addr;
}

static void *vio_modern_map_simple_capability(VirtIODevice *vdev, int cap_offset, size_t length, u32 alignment)
{
    return vio_modern_map_capability(
        vdev,
        cap_offset,
        length,      // minlen
        alignment,
        0,           // offset
        (u32)length, // size is equal to minlen
        NULL);       // not interested in the full length
}

static void vio_modern_get_config(VirtIODevice *vdev, unsigned offset,
                                  void *buf, unsigned len)
{
    if (!vdev->config) {
        ASSERT(!"Device has no config to read");
        return;
    }
    if (offset + len > vdev->config_len) {
        ASSERT(!"Can't read beyond the config length");
        return;
    }

    switch (len) {
    case 1:
        *(u8 *)buf = ioread8(vdev, vdev->config + offset);
        break;
    case 2:
        *(u16 *)buf = ioread16(vdev, vdev->config + offset);
        break;
    case 4:
        *(u32 *)buf = ioread32(vdev, vdev->config + offset);
        break;
    default:
        ASSERT(!"Only 1, 2, 4 byte config reads are supported");
    }
}

static void vio_modern_set_config(VirtIODevice *vdev, unsigned offset,
                                  const void *buf, unsigned len)
{
    if (!vdev->config) {
        ASSERT(!"Device has no config to write");
        return;
    }
    if (offset + len > vdev->config_len) {
        ASSERT(!"Can't write beyond the config length");
        return;
    }

    switch (len) {
    case 1:
        iowrite8(vdev, *(u8 *)buf, vdev->config + offset);
        break;
    case 2:
        iowrite16(vdev, *(u16 *)buf, vdev->config + offset);
        break;
    case 4:
        iowrite32(vdev, *(u32 *)buf, vdev->config + offset);
        break;
    default:
        ASSERT(!"Only 1, 2, 4 byte config writes are supported");
    }
}

static u32 vio_modern_get_generation(VirtIODevice *vdev)
{
    return ioread8(vdev, &vdev->common->config_generation);
}

static u8 vio_modern_get_status(VirtIODevice *vdev)
{
    return ioread8(vdev, &vdev->common->device_status);
}

static void vio_modern_set_status(VirtIODevice *vdev, u8 status)
{
    /* We should never be setting status to 0. */
    ASSERT(status != 0);
    iowrite8(vdev, status, &vdev->common->device_status);
}

static void vio_modern_reset(VirtIODevice *vdev)
{
    /* 0 status means a reset. */
    iowrite8(vdev, 0, &vdev->common->device_status);
    /* After writing 0 to device_status, the driver MUST wait for a read of
     * device_status to return 0 before reinitializing the device.
     * This will flush out the status write, and flush in device writes,
     * including MSI-X interrupts, if any.
     */
    while (ioread8(vdev, &vdev->common->device_status)) {
        vdev_sleep(vdev, 1);
    }
}

static u64 vio_modern_get_features(VirtIODevice *vdev)
{
    u64 features;

    iowrite32(vdev, 0, &vdev->common->device_feature_select);
    features = ioread32(vdev, &vdev->common->device_feature);
    iowrite32(vdev, 1, &vdev->common->device_feature_select);
    features |= ((u64)ioread32(vdev, &vdev->common->device_feature) << 32);

    return features;
}

static NTSTATUS vio_modern_set_features(VirtIODevice *vdev, u64 features)
{
    /* Give virtio_ring a chance to accept features. */
    vring_transport_features(vdev, &features);

    if (!virtio_is_feature_enabled(features, VIRTIO_F_VERSION_1)) {
        DPrintf(0, "virtio: device uses modern interface but does not have VIRTIO_F_VERSION_1\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    iowrite32(vdev, 0, &vdev->common->guest_feature_select);
    iowrite32(vdev, (u32)features, &vdev->common->guest_feature);
    iowrite32(vdev, 1, &vdev->common->guest_feature_select);
    iowrite32(vdev, features >> 32, &vdev->common->guest_feature);

    return STATUS_SUCCESS;
}

static u16 vio_modern_set_config_vector(VirtIODevice *vdev, u16 vector)
{
    /* Setup the vector used for configuration events */
    iowrite16(vdev, vector, &vdev->common->msix_config);
    /* Verify we had enough resources to assign the vector */
    /* Will also flush the write out to device */
    return ioread16(vdev, &vdev->common->msix_config);
}

static u16 vio_modern_set_queue_vector(struct virtqueue *vq, u16 vector)
{
    VirtIODevice *vdev = vq->vdev;
    volatile struct virtio_pci_common_cfg *cfg = vdev->common;

    iowrite16(vdev, (u16)vq->index, &cfg->queue_select);
    iowrite16(vdev, vector, &cfg->queue_msix_vector);
    return ioread16(vdev, &cfg->queue_msix_vector);
}

static size_t vring_pci_size(u16 num, bool packed)
{
    /* We only need a cacheline separation. */
    return (size_t)ROUND_TO_PAGES(vring_size(num, SMP_CACHE_BYTES, packed));
}

static NTSTATUS vio_modern_query_vq_alloc(VirtIODevice *vdev,
                                          unsigned index,
                                          unsigned short *pNumEntries,
                                          unsigned long *pRingSize,
                                          unsigned long *pHeapSize)
{
    volatile struct virtio_pci_common_cfg *cfg = vdev->common;
    u16 num;

    if (index >= ioread16(vdev, &cfg->num_queues)) {
        return STATUS_NOT_FOUND;
    }

    /* Select the queue we're interested in */
    iowrite16(vdev, (u16)index, &cfg->queue_select);

    /* Check if queue is either not available or already active. */
    num = ioread16(vdev, &cfg->queue_size);
    /* QEMU has a bug where queues don't revert to inactive on device
     * reset. Skip checking the queue_enable field until it is fixed.
     */
    if (!num /*|| ioread16(vdev, &cfg->queue_enable)*/) {
        return STATUS_NOT_FOUND;
    }

    if (num & (num - 1)) {
        DPrintf(0, "%p: bad queue size %u", vdev, num);
        return STATUS_INVALID_PARAMETER;
    }

    *pNumEntries = num;
    *pRingSize = (unsigned long)vring_pci_size(num, vdev->packed_ring);
    *pHeapSize = vring_control_block_size(num, vdev->packed_ring);

    return STATUS_SUCCESS;
}

static NTSTATUS vio_modern_setup_vq(struct virtqueue **queue,
                                    VirtIODevice *vdev,
                                    VirtIOQueueInfo *info,
                                    unsigned index,
                                    u16 msix_vec)
{
    volatile struct virtio_pci_common_cfg *cfg = vdev->common;
    struct virtqueue *vq;
    void *vq_addr;
    u16 off;
    unsigned long ring_size, heap_size;
    NTSTATUS status;

    /* select the queue and query allocation parameters */
    status = vio_modern_query_vq_alloc(vdev, index, &info->num, &ring_size, &heap_size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* get offset of notification word for this vq */
    off = ioread16(vdev, &cfg->queue_notify_off);

    /* try to allocate contiguous pages, scale down on failure */
    while (!(info->queue = mem_alloc_contiguous_pages(vdev, vring_pci_size(info->num, vdev->packed_ring)))) {
        if (info->num > 0) {
            info->num /= 2;
        } else {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    vq_addr = mem_alloc_nonpaged_block(vdev, heap_size);
    if (vq_addr == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* create the vring */
    if (vdev->packed_ring) {
        vq = vring_new_virtqueue_packed(index, info->num,
            SMP_CACHE_BYTES, vdev,
            info->queue, vp_notify, vq_addr);
    } else {
        vq = vring_new_virtqueue_split(index, info->num,
            SMP_CACHE_BYTES, vdev,
            info->queue, vp_notify, vq_addr);
    }

    if (!vq) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto err_new_queue;
    }

    /* activate the queue */
    iowrite16(vdev, info->num, &cfg->queue_size);
    iowrite64_twopart(vdev, mem_get_physical_address(vdev, info->queue),
        &cfg->queue_desc_lo, &cfg->queue_desc_hi);
    iowrite64_twopart(vdev, mem_get_physical_address(vdev, vq->avail_va),
        &cfg->queue_avail_lo, &cfg->queue_avail_hi);
    iowrite64_twopart(vdev, mem_get_physical_address(vdev, vq->used_va),
        &cfg->queue_used_lo, &cfg->queue_used_hi);

    if (vdev->notify_base) {
        /* offset should not wrap */
        if ((u64)off * vdev->notify_offset_multiplier + 2
            > vdev->notify_len) {
            DPrintf(0,
                "%p: bad notification offset %u (x %u) "
                "for queue %u > %zd",
                vdev,
                off, vdev->notify_offset_multiplier,
                index, vdev->notify_len);
            status = STATUS_INVALID_PARAMETER;
            goto err_map_notify;
        }
        vq->notification_addr = (void *)(vdev->notify_base +
            off * vdev->notify_offset_multiplier);
    } else {
        vq->notification_addr = vio_modern_map_capability(vdev,
            vdev->notify_map_cap, 2, 2,
            off * vdev->notify_offset_multiplier, 2,
            NULL);
    }

    if (!vq->notification_addr) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto err_map_notify;
    }

    if (msix_vec != VIRTIO_MSI_NO_VECTOR) {
        msix_vec = vdev->device->set_queue_vector(vq, msix_vec);
        if (msix_vec == VIRTIO_MSI_NO_VECTOR) {
            status = STATUS_DEVICE_BUSY;
            goto err_assign_vector;
        }
    }

    /* enable the queue */
    iowrite16(vdev, 1, &vdev->common->queue_enable);

    *queue = vq;
    return STATUS_SUCCESS;

err_assign_vector:
err_map_notify:
    virtqueue_shutdown(vq);
err_new_queue:
    mem_free_nonpaged_block(vdev, vq_addr);
    mem_free_contiguous_pages(vdev, info->queue);
    return status;
}

static void vio_modern_del_vq(VirtIOQueueInfo *info)
{
    struct virtqueue *vq = info->vq;
    VirtIODevice *vdev = vq->vdev;

    iowrite16(vdev, (u16)vq->index, &vdev->common->queue_select);

    if (vdev->msix_used) {
        iowrite16(vdev, VIRTIO_MSI_NO_VECTOR, &vdev->common->queue_msix_vector);
        /* Flush the write out to device */
        ioread16(vdev, &vdev->common->queue_msix_vector);
    }

    virtqueue_shutdown(vq);

    mem_free_nonpaged_block(vdev, vq);
    mem_free_contiguous_pages(vdev, info->queue);
}

static const struct virtio_device_ops virtio_pci_device_ops = {
    /* .get_config = */ vio_modern_get_config,
    /* .set_config = */ vio_modern_set_config,
    /* .get_config_generation = */ vio_modern_get_generation,
    /* .get_status = */ vio_modern_get_status,
    /* .set_status = */ vio_modern_set_status,
    /* .reset = */ vio_modern_reset,
    /* .get_features = */ vio_modern_get_features,
    /* .set_features = */ vio_modern_set_features,
    /* .set_config_vector = */ vio_modern_set_config_vector,
    /* .set_queue_vector = */ vio_modern_set_queue_vector,
    /* .query_queue_alloc = */ vio_modern_query_vq_alloc,
    /* .setup_queue = */ vio_modern_setup_vq,
    /* .delete_queue = */ vio_modern_del_vq,
};

static u8 find_next_pci_vendor_capability(VirtIODevice *vdev, u8 offset)
{
    u8 id = 0;
    int iterations = 48;

    if (pci_read_config_byte(vdev, offset, &offset) != 0) {
        return 0;
    }

    while (iterations-- && offset >= 0x40) {
        offset &= ~3;
        if (pci_read_config_byte(vdev, offset + offsetof(PCI_CAPABILITIES_HEADER,
                CapabilityID), &id) != 0) {
            break;
        }
        if (id == 0xFF) {
            break;
        }
        if (id == PCI_CAPABILITY_ID_VENDOR_SPECIFIC) {
            return offset;
        }
        if (pci_read_config_byte(vdev, offset + offsetof(PCI_CAPABILITIES_HEADER,
                Next), &offset) != 0) {
            break;
        }
    }
    return 0;
}

static u8 find_first_pci_vendor_capability(VirtIODevice *vdev)
{
    u8 hdr_type, offset;
    u16 status;

    if (pci_read_config_byte(vdev, offsetof(PCI_COMMON_HEADER, HeaderType), &hdr_type) != 0) {
        return 0;
    }
    if (pci_read_config_word(vdev, offsetof(PCI_COMMON_HEADER, Status), &status) != 0) {
        return 0;
    }
    if ((status & PCI_STATUS_CAPABILITIES_LIST) == 0) {
        return 0;
    }

    switch (hdr_type & ~PCI_MULTIFUNCTION) {
    case PCI_BRIDGE_TYPE:
        offset = offsetof(PCI_COMMON_HEADER, u.type1.CapabilitiesPtr);
        break;
    case PCI_CARDBUS_BRIDGE_TYPE:
        offset = offsetof(PCI_COMMON_HEADER, u.type2.CapabilitiesPtr);
        break;
    default:
        offset = offsetof(PCI_COMMON_HEADER, u.type0.CapabilitiesPtr);
        break;
    }

    if (offset != 0) {
        offset = find_next_pci_vendor_capability(vdev, offset);
    }
    return offset;
}

/* Populate Offsets with virtio vendor capability offsets within the PCI config space */
static void find_pci_vendor_capabilities(VirtIODevice *vdev, int *Offsets, size_t nOffsets)
{
    u8 offset = find_first_pci_vendor_capability(vdev);
    while (offset > 0) {
        u8 cfg_type, bar;
        pci_read_config_byte(vdev, offset + offsetof(struct virtio_pci_cap, cfg_type), &cfg_type);
        pci_read_config_byte(vdev, offset + offsetof(struct virtio_pci_cap, bar), &bar);

        if (bar < PCI_TYPE0_ADDRESSES &&
            cfg_type < nOffsets &&
            pci_get_resource_len(vdev, bar) > 0) {
            Offsets[cfg_type] = offset;
        }

        offset = find_next_pci_vendor_capability(vdev, offset + offsetof(PCI_CAPABILITIES_HEADER, Next));
    }
}

/* Modern device initialization */
NTSTATUS vio_modern_initialize(VirtIODevice *vdev)
{
    int capabilities[VIRTIO_PCI_CAP_PCI_CFG];

    u32 notify_length;
    u32 notify_offset;

    RtlZeroMemory(capabilities, sizeof(capabilities));
    find_pci_vendor_capabilities(vdev, capabilities, VIRTIO_PCI_CAP_PCI_CFG);

    /* Check for a common config, if not found use legacy mode */
    if (!capabilities[VIRTIO_PCI_CAP_COMMON_CFG]) {
        DPrintf(0, "%s(%p): device not found\n", __FUNCTION__, vdev);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    /* Check isr and notify caps, if not found fail */
    if (!capabilities[VIRTIO_PCI_CAP_ISR_CFG] || !capabilities[VIRTIO_PCI_CAP_NOTIFY_CFG]) {
        DPrintf(0, "%s(%p): missing capabilities %i/%i/%i\n",
            __FUNCTION__, vdev,
            capabilities[VIRTIO_PCI_CAP_COMMON_CFG],
            capabilities[VIRTIO_PCI_CAP_ISR_CFG],
            capabilities[VIRTIO_PCI_CAP_NOTIFY_CFG]);
        return STATUS_INVALID_PARAMETER;
    }

    /* Map bars according to the capabilities */
    vdev->common = vio_modern_map_simple_capability(vdev,
        capabilities[VIRTIO_PCI_CAP_COMMON_CFG],
        sizeof(struct virtio_pci_common_cfg), 4);
    if (!vdev->common) {
        return STATUS_INVALID_PARAMETER;
    }

    vdev->isr = vio_modern_map_simple_capability(vdev,
        capabilities[VIRTIO_PCI_CAP_ISR_CFG],
        sizeof(u8), 1);
    if (!vdev->isr) {
        return STATUS_INVALID_PARAMETER;
    }

    /* Read notify_off_multiplier from config space. */
    pci_read_config_dword(vdev,
        capabilities[VIRTIO_PCI_CAP_NOTIFY_CFG] + offsetof(struct virtio_pci_notify_cap,
        notify_off_multiplier),
        &vdev->notify_offset_multiplier);

    /* Read notify length and offset from config space. */
    pci_read_config_dword(vdev,
        capabilities[VIRTIO_PCI_CAP_NOTIFY_CFG] + offsetof(struct virtio_pci_notify_cap,
        cap.length),
        &notify_length);
    pci_read_config_dword(vdev,
        capabilities[VIRTIO_PCI_CAP_NOTIFY_CFG] + offsetof(struct virtio_pci_notify_cap,
        cap.offset),
        &notify_offset);

    /* Map the notify capability if it's small enough.
     * Otherwise, map each VQ individually later.
     */
    if (notify_length + (notify_offset % PAGE_SIZE) <= PAGE_SIZE) {
        vdev->notify_base = vio_modern_map_capability(vdev,
            capabilities[VIRTIO_PCI_CAP_NOTIFY_CFG], 2, 2,
            0, notify_length,
            &vdev->notify_len);
        if (!vdev->notify_base) {
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        vdev->notify_map_cap = capabilities[VIRTIO_PCI_CAP_NOTIFY_CFG];
    }

    /* Map the device config capability, the PAGE_SIZE size is a guess */
    if (capabilities[VIRTIO_PCI_CAP_DEVICE_CFG]) {
        vdev->config = vio_modern_map_capability(vdev,
            capabilities[VIRTIO_PCI_CAP_DEVICE_CFG], 0, 4,
            0, PAGE_SIZE,
            &vdev->config_len);
        if (!vdev->config) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    vdev->device = &virtio_pci_device_ops;

    return STATUS_SUCCESS;
}
