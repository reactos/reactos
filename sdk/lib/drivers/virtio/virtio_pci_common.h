#ifndef _DRIVERS_VIRTIO_VIRTIO_PCI_COMMON_H
#define _DRIVERS_VIRTIO_VIRTIO_PCI_COMMON_H
/*
 * Virtio PCI driver - APIs for common functionality for all device versions
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

#define ioread8(vdev, addr) \
    vdev->system->vdev_read_byte((ULONG_PTR)(addr))
#define ioread16(vdev, addr) \
    vdev->system->vdev_read_word((ULONG_PTR)(addr))
#define ioread32(vdev, addr) \
    vdev->system->vdev_read_dword((ULONG_PTR)(addr))
#define iowrite8(vdev, val, addr) \
    vdev->system->vdev_write_byte((ULONG_PTR)(addr), val)
#define iowrite16(vdev, val, addr) \
    vdev->system->vdev_write_word((ULONG_PTR)(addr), val)
#define iowrite32(vdev, val, addr) \
    vdev->system->vdev_write_dword((ULONG_PTR)(addr), val)
#define iowrite64_twopart(vdev, val, lo_addr, hi_addr) \
    vdev->system->vdev_write_dword((ULONG_PTR)(lo_addr), (u32)(val)); \
    vdev->system->vdev_write_dword((ULONG_PTR)(hi_addr), (val) >> 32)

#define mem_alloc_contiguous_pages(vdev, size) \
    vdev->system->mem_alloc_contiguous_pages(vdev->DeviceContext, size)
#define mem_free_contiguous_pages(vdev, virt) \
    vdev->system->mem_free_contiguous_pages(vdev->DeviceContext, virt)
#define mem_get_physical_address(vdev, virt) \
    vdev->system->mem_get_physical_address(vdev->DeviceContext, virt)
#define mem_alloc_nonpaged_block(vdev, size) \
    vdev->system->mem_alloc_nonpaged_block(vdev->DeviceContext, size)
#define mem_free_nonpaged_block(vdev, addr) \
    vdev->system->mem_free_nonpaged_block(vdev->DeviceContext, addr)

#define pci_read_config_byte(vdev, where, bVal) \
    vdev->system->pci_read_config_byte(vdev->DeviceContext, where, bVal)
#define pci_read_config_word(vdev, where, wVal) \
    vdev->system->pci_read_config_word(vdev->DeviceContext, where, wVal)
#define pci_read_config_dword(vdev, where, dwVal) \
    vdev->system->pci_read_config_dword(vdev->DeviceContext, where, dwVal)

#define pci_get_resource_len(vdev, bar) \
    vdev->system->pci_get_resource_len(vdev->DeviceContext, bar)
#define pci_map_address_range(vdev, bar, offset, maxlen) \
    vdev->system->pci_map_address_range(vdev->DeviceContext, bar, offset, maxlen)

#define vdev_get_msix_vector(vdev, queue) \
    vdev->system->vdev_get_msix_vector(vdev->DeviceContext, queue)
#define vdev_sleep(vdev, msecs) \
    vdev->system->vdev_sleep(vdev->DeviceContext, msecs)

/* the notify function used when creating a virt queue */
void vp_notify(struct virtqueue *vq);

NTSTATUS vio_legacy_initialize(VirtIODevice *vdev);
NTSTATUS vio_modern_initialize(VirtIODevice *vdev);

#endif
