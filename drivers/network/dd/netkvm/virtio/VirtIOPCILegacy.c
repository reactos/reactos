/*
 * Virtio PCI driver - legacy (virtio 0.9) device support
 *
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Anthony Liguori  <aliguori@us.ibm.com>
 *  Windows porting - Yan Vugenfirer <yvugenfi@redhat.com>
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
#include "virtio_pci_common.h"
#include "windows/virtio_ring_allocation.h"

#ifdef WPP_EVENT_TRACING
#include "VirtIOPCILegacy.tmh"
#endif

/////////////////////////////////////////////////////////////////////////////////////
//
// vio_legacy_dump_registers - Dump HW registers of the device
//
/////////////////////////////////////////////////////////////////////////////////////
void vio_legacy_dump_registers(VirtIODevice *vdev)
{
    DPrintf(5, "%s\n", __FUNCTION__);

    DPrintf(0, "[VIRTIO_PCI_HOST_FEATURES] = %x\n", ioread32(vdev, vdev->addr + VIRTIO_PCI_HOST_FEATURES));
    DPrintf(0, "[VIRTIO_PCI_GUEST_FEATURES] = %x\n", ioread32(vdev, vdev->addr + VIRTIO_PCI_GUEST_FEATURES));
    DPrintf(0, "[VIRTIO_PCI_QUEUE_PFN] = %x\n", ioread32(vdev, vdev->addr + VIRTIO_PCI_QUEUE_PFN));
    DPrintf(0, "[VIRTIO_PCI_QUEUE_NUM] = %x\n", ioread32(vdev, vdev->addr + VIRTIO_PCI_QUEUE_NUM));
    DPrintf(0, "[VIRTIO_PCI_QUEUE_SEL] = %x\n", ioread32(vdev, vdev->addr + VIRTIO_PCI_QUEUE_SEL));
    DPrintf(0, "[VIRTIO_PCI_QUEUE_NOTIFY] = %x\n", ioread32(vdev, vdev->addr + VIRTIO_PCI_QUEUE_NOTIFY));
    DPrintf(0, "[VIRTIO_PCI_STATUS] = %x\n", ioread32(vdev, vdev->addr + VIRTIO_PCI_STATUS));
    DPrintf(0, "[VIRTIO_PCI_ISR] = %x\n", ioread32(vdev, vdev->addr + VIRTIO_PCI_ISR));
}

static void vio_legacy_get_config(VirtIODevice * vdev,
                                  unsigned offset,
                                  void *buf,
                                  unsigned len)
{
    ULONG_PTR ioaddr = vdev->addr + VIRTIO_PCI_CONFIG(vdev->msix_used) + offset;
    u8 *ptr = buf;
    unsigned i;

    DPrintf(5, "%s\n", __FUNCTION__);

    for (i = 0; i < len; i++) {
        ptr[i] = ioread8(vdev, ioaddr + i);
    }
}

static void vio_legacy_set_config(VirtIODevice *vdev,
                                  unsigned offset,
                                  const void *buf,
                                  unsigned len)
{
    ULONG_PTR ioaddr = vdev->addr + VIRTIO_PCI_CONFIG(vdev->msix_used) + offset;
    const u8 *ptr = buf;
    unsigned i;

    DPrintf(5, "%s\n", __FUNCTION__);

    for (i = 0; i < len; i++) {
        iowrite8(vdev, ptr[i], ioaddr + i);
    }
}

static u8 vio_legacy_get_status(VirtIODevice *vdev)
{
    DPrintf(6, "%s\n", __FUNCTION__);
    return ioread8(vdev, vdev->addr + VIRTIO_PCI_STATUS);
}

static void vio_legacy_set_status(VirtIODevice *vdev, u8 status)
{
    DPrintf(6, "%s>>> %x\n", __FUNCTION__, status);
    iowrite8(vdev, status, vdev->addr + VIRTIO_PCI_STATUS);
}

static void vio_legacy_reset(VirtIODevice *vdev)
{
    /* 0 status means a reset. */
    iowrite8(vdev, 0, vdev->addr + VIRTIO_PCI_STATUS);
}

static u64 vio_legacy_get_features(VirtIODevice *vdev)
{
    return ioread32(vdev, vdev->addr + VIRTIO_PCI_HOST_FEATURES);
}

static NTSTATUS vio_legacy_set_features(VirtIODevice *vdev, u64 features)
{
    /* Give virtio_ring a chance to accept features. */
    vring_transport_features(vdev, &features);

    /* Make sure we don't have any features > 32 bits! */
    ASSERT((u32)features == features);
    iowrite32(vdev, (u32)features, vdev->addr + VIRTIO_PCI_GUEST_FEATURES);

    return STATUS_SUCCESS;
}

static u16 vio_legacy_set_config_vector(VirtIODevice *vdev, u16 vector)
{
    /* Setup the vector used for configuration events */
    iowrite16(vdev, vector, vdev->addr + VIRTIO_MSI_CONFIG_VECTOR);
    /* Verify we had enough resources to assign the vector */
    /* Will also flush the write out to device */
    return ioread16(vdev, vdev->addr + VIRTIO_MSI_CONFIG_VECTOR);
}

static u16 vio_legacy_set_queue_vector(struct virtqueue *vq, u16 vector)
{
    VirtIODevice *vdev = vq->vdev;

    iowrite16(vdev, (u16)vq->index, vdev->addr + VIRTIO_PCI_QUEUE_SEL);
    iowrite16(vdev, vector, vdev->addr + VIRTIO_MSI_QUEUE_VECTOR);
    return ioread16(vdev, vdev->addr + VIRTIO_MSI_QUEUE_VECTOR);
}

static NTSTATUS vio_legacy_query_vq_alloc(VirtIODevice *vdev,
                                          unsigned index,
                                          unsigned short *pNumEntries,
                                          unsigned long *pRingSize,
                                          unsigned long *pHeapSize)
{
    unsigned long ring_size, data_size;
    u16 num;

    /* Select the queue we're interested in */
    iowrite16(vdev, (u16)index, vdev->addr + VIRTIO_PCI_QUEUE_SEL);

    /* Check if queue is either not available or already active. */
    num = ioread16(vdev, vdev->addr + VIRTIO_PCI_QUEUE_NUM);
    if (!num || ioread32(vdev, vdev->addr + VIRTIO_PCI_QUEUE_PFN)) {
        return STATUS_NOT_FOUND;
    }

    ring_size = ROUND_TO_PAGES(vring_size(num, VIRTIO_PCI_VRING_ALIGN, false));
    data_size = ROUND_TO_PAGES(vring_control_block_size(num, false));

    *pNumEntries = num;
    *pRingSize = ring_size + data_size;
    *pHeapSize = 0;

    return STATUS_SUCCESS;
}

static NTSTATUS vio_legacy_setup_vq(struct virtqueue **queue,
                                    VirtIODevice *vdev,
                                    VirtIOQueueInfo *info,
                                    unsigned index,
                                    u16 msix_vec)
{
    struct virtqueue *vq;
    unsigned long ring_size, heap_size;
    NTSTATUS status;

    /* Select the queue and query allocation parameters */
    status = vio_legacy_query_vq_alloc(vdev, index, &info->num, &ring_size, &heap_size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    info->queue = mem_alloc_contiguous_pages(vdev, ring_size);
    if (info->queue == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* activate the queue */
    iowrite32(vdev, (u32)(mem_get_physical_address(vdev, info->queue) >> VIRTIO_PCI_QUEUE_ADDR_SHIFT),
        vdev->addr + VIRTIO_PCI_QUEUE_PFN);

    /* create the vring */
    vq = vring_new_virtqueue_split(index, info->num,
        VIRTIO_PCI_VRING_ALIGN, vdev,
        info->queue, vp_notify,
        (u8 *)info->queue + ROUND_TO_PAGES(vring_size(info->num, VIRTIO_PCI_VRING_ALIGN, false)));
    if (!vq) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto err_activate_queue;
    }

    vq->notification_addr = (void *)(vdev->addr + VIRTIO_PCI_QUEUE_NOTIFY);

    if (msix_vec != VIRTIO_MSI_NO_VECTOR) {
        msix_vec = vdev->device->set_queue_vector(vq, msix_vec);
        if (msix_vec == VIRTIO_MSI_NO_VECTOR) {
            status = STATUS_DEVICE_BUSY;
            goto err_assign;
        }
    }

    *queue = vq;
    return STATUS_SUCCESS;

err_assign:
err_activate_queue:
    iowrite32(vdev, 0, vdev->addr + VIRTIO_PCI_QUEUE_PFN);
    mem_free_contiguous_pages(vdev, info->queue);
    return status;
}

static void vio_legacy_del_vq(VirtIOQueueInfo *info)
{
    struct virtqueue *vq = info->vq;
    VirtIODevice *vdev = vq->vdev;

    iowrite16(vdev, (u16)vq->index, vdev->addr + VIRTIO_PCI_QUEUE_SEL);

    if (vdev->msix_used) {
        iowrite16(vdev, VIRTIO_MSI_NO_VECTOR,
            vdev->addr + VIRTIO_MSI_QUEUE_VECTOR);
        /* Flush the write out to device */
        ioread8(vdev, vdev->addr + VIRTIO_PCI_ISR);
    }

    /* Select and deactivate the queue */
    iowrite32(vdev, 0, vdev->addr + VIRTIO_PCI_QUEUE_PFN);

    mem_free_contiguous_pages(vdev, info->queue);
}

static const struct virtio_device_ops virtio_pci_device_ops = {
    /* .get_config = */ vio_legacy_get_config,
    /* .set_config = */ vio_legacy_set_config,
    /* .get_config_generation = */ NULL,
    /* .get_status = */ vio_legacy_get_status,
    /* .set_status = */ vio_legacy_set_status,
    /* .reset = */ vio_legacy_reset,
    /* .get_features = */ vio_legacy_get_features,
    /* .set_features = */ vio_legacy_set_features,
    /* .set_config_vector = */ vio_legacy_set_config_vector,
    /* .set_queue_vector = */ vio_legacy_set_queue_vector,
    /* .query_queue_alloc = */ vio_legacy_query_vq_alloc,
    /* .setup_queue = */ vio_legacy_setup_vq,
    /* .delete_queue = */ vio_legacy_del_vq,
};

/* Legacy device initialization */
NTSTATUS vio_legacy_initialize(VirtIODevice *vdev)
{
    size_t length = pci_get_resource_len(vdev, 0);
    vdev->addr = (ULONG_PTR)pci_map_address_range(vdev, 0, 0, length);

    if (!vdev->addr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    vdev->isr = (u8 *)vdev->addr + VIRTIO_PCI_ISR;

    vdev->device = &virtio_pci_device_ops;

    return STATUS_SUCCESS;
}
