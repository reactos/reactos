/*
* Virtio PCI driver
*
* This module allows virtio devices to be used over a virtual PCI device.
* This can be used with QEMU based VMMs like KVM or Xen.
*
* Copyright IBM Corp. 2007
*
* Authors:
*  Anthony Liguori  <aliguori@us.ibm.com>
*
* This header is BSD licensed so anyone can use the definitions to implement
* compatible drivers/servers.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of IBM nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/

#ifndef _LINUX_VIRTIO_PCI_H
#define _LINUX_VIRTIO_PCI_H

#include "linux/types.h"
#include "linux/virtio_config.h"

#ifndef VIRTIO_PCI_NO_LEGACY

/* A 32-bit r/o bitmask of the features supported by the host */
#define VIRTIO_PCI_HOST_FEATURES    0

/* A 32-bit r/w bitmask of features activated by the guest */
#define VIRTIO_PCI_GUEST_FEATURES   4

/* A 32-bit r/w PFN for the currently selected queue */
#define VIRTIO_PCI_QUEUE_PFN        8

/* A 16-bit r/o queue size for the currently selected queue */
#define VIRTIO_PCI_QUEUE_NUM        12

/* A 16-bit r/w queue selector */
#define VIRTIO_PCI_QUEUE_SEL        14

/* A 16-bit r/w queue notifier */
#define VIRTIO_PCI_QUEUE_NOTIFY     16

/* An 8-bit device status register.  */
#define VIRTIO_PCI_STATUS           18

/* An 8-bit r/o interrupt status register.  Reading the value will return the
* current contents of the ISR and will also clear it.  This is effectively
* a read-and-acknowledge. */
#define VIRTIO_PCI_ISR              19

/* MSI-X registers: only enabled if MSI-X is enabled. */
/* A 16-bit vector for configuration changes. */
#define VIRTIO_MSI_CONFIG_VECTOR    20
/* A 16-bit vector for selected queue notifications. */
#define VIRTIO_MSI_QUEUE_VECTOR     22

/* The remaining space is defined by each driver as the per-driver
* configuration space */
#define VIRTIO_PCI_CONFIG_OFF(msix_enabled) ((msix_enabled) ? 24 : 20)
/* Deprecated: please use VIRTIO_PCI_CONFIG_OFF instead */
#define VIRTIO_PCI_CONFIG(msix_enabled) VIRTIO_PCI_CONFIG_OFF(msix_enabled)

/* How many bits to shift physical queue address written to QUEUE_PFN.
* 12 is historical, and due to x86 page size. */
#define VIRTIO_PCI_QUEUE_ADDR_SHIFT 12

/* The alignment to use between consumer and producer parts of vring.
* x86 pagesize again. */
#define VIRTIO_PCI_VRING_ALIGN      4096

#endif /* VIRTIO_PCI_NO_LEGACY */

/* The bit of the ISR which indicates a device configuration change. */
#define VIRTIO_PCI_ISR_CONFIG       0x2
/* Vector value used to disable MSI for queue */
#define VIRTIO_MSI_NO_VECTOR        0xffff

/* IDs for different capabilities.  Must all exist. */

/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG   1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG   2
/* ISR access */
#define VIRTIO_PCI_CAP_ISR_CFG      3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG   4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG      5

/* This is the PCI capability header: */
struct virtio_pci_cap {
    __u8 cap_vndr;      /* Generic PCI field: PCI_CAPABILITY_ID_VENDOR_SPECIFIC */
    __u8 cap_next;      /* Generic PCI field: next ptr. */
    __u8 cap_len;       /* Generic PCI field: capability length */
    __u8 cfg_type;      /* Identifies the structure. */
    __u8 bar;           /* Where to find it. */
    __u8 padding[3];    /* Pad to full dword. */
    __le32 offset;      /* Offset within bar. */
    __le32 length;      /* Length of the structure, in bytes. */
};

struct virtio_pci_notify_cap {
    struct virtio_pci_cap cap;
    __le32 notify_off_multiplier;   /* Multiplier for queue_notify_off. */
};

/* Fields in VIRTIO_PCI_CAP_COMMON_CFG: */
struct virtio_pci_common_cfg {
    /* About the whole device. */
    __le32 device_feature_select;   /* read-write */
    __le32 device_feature;          /* read-only */
    __le32 guest_feature_select;    /* read-write */
    __le32 guest_feature;           /* read-write */
    __le16 msix_config;             /* read-write */
    __le16 num_queues;              /* read-only */
    __u8 device_status;             /* read-write */
    __u8 config_generation;         /* read-only */

    /* About a specific virtqueue. */
    __le16 queue_select;            /* read-write */
    __le16 queue_size;              /* read-write, power of 2. */
    __le16 queue_msix_vector;       /* read-write */
    __le16 queue_enable;            /* read-write */
    __le16 queue_notify_off;        /* read-only */
    __le32 queue_desc_lo;           /* read-write */
    __le32 queue_desc_hi;           /* read-write */
    __le32 queue_avail_lo;          /* read-write */
    __le32 queue_avail_hi;          /* read-write */
    __le32 queue_used_lo;           /* read-write */
    __le32 queue_used_hi;           /* read-write */
};

#define MAX_QUEUES_PER_DEVICE_DEFAULT 8

typedef struct virtio_queue_info
{
    /* the actual virtqueue */
    struct virtqueue *vq;
    /* the number of entries in the queue */
    u16 num;
    /* the virtual address of the ring queue */
    void *queue;
} VirtIOQueueInfo;

typedef struct virtio_system_ops {
    // device register access
    u8 (*vdev_read_byte)(ULONG_PTR ulRegister);
    u16 (*vdev_read_word)(ULONG_PTR ulRegister);
    u32 (*vdev_read_dword)(ULONG_PTR ulRegister);
    void (*vdev_write_byte)(ULONG_PTR ulRegister, u8 bValue);
    void (*vdev_write_word)(ULONG_PTR ulRegister, u16 wValue);
    void (*vdev_write_dword)(ULONG_PTR ulRegister, u32 ulValue);

    // memory management
    void *(*mem_alloc_contiguous_pages)(void *context, size_t size);
    void (*mem_free_contiguous_pages)(void *context, void *virt);
    ULONGLONG (*mem_get_physical_address)(void *context, void *virt);
    void *(*mem_alloc_nonpaged_block)(void *context, size_t size);
    void (*mem_free_nonpaged_block)(void *context, void *addr);

    // PCI config space access
    int (*pci_read_config_byte)(void *context, int where, u8 *bVal);
    int (*pci_read_config_word)(void *context, int where, u16 *wVal);
    int (*pci_read_config_dword)(void *context, int where, u32 *dwVal);

    // PCI resource handling
    size_t (*pci_get_resource_len)(void *context, int bar);
    void *(*pci_map_address_range)(void *context, int bar, size_t offset, size_t maxlen);

    // misc
    u16 (*vdev_get_msix_vector)(void *context, int queue);
    void (*vdev_sleep)(void *context, unsigned int msecs);
} VirtIOSystemOps;

struct virtio_device;
typedef struct virtio_device VirtIODevice;

struct virtio_device_ops
{
    // read/write device config and read config generation counter
    void (*get_config)(VirtIODevice *vdev, unsigned offset, void *buf, unsigned len);
    void (*set_config)(VirtIODevice *vdev, unsigned offset, const void *buf, unsigned len);
    u32 (*get_config_generation)(VirtIODevice *vdev);

    // read/write device status byte and reset the device
    u8 (*get_status)(VirtIODevice *vdev);
    void (*set_status)(VirtIODevice *vdev, u8 status);
    void (*reset)(VirtIODevice *vdev);

    // get/set device feature bits
    u64 (*get_features)(VirtIODevice *vdev);
    NTSTATUS (*set_features)(VirtIODevice *vdev, u64 features);

    // set config/queue MSI interrupt vector, returns the new vector
    u16 (*set_config_vector)(VirtIODevice *vdev, u16 vector);
    u16 (*set_queue_vector)(struct virtqueue *vq, u16 vector);

    // query virtual queue size and memory requirements
    NTSTATUS (*query_queue_alloc)(VirtIODevice *vdev,
        unsigned index, unsigned short *pNumEntries,
        unsigned long *pRingSize,
        unsigned long *pHeapSize);

    // allocate and initialize a queue
    NTSTATUS (*setup_queue)(struct virtqueue **queue,
        VirtIODevice *vdev, VirtIOQueueInfo *info,
        unsigned idx, u16 msix_vec);

    // tear down and deallocate a queue
    void (*delete_queue)(VirtIOQueueInfo *info);
};

struct virtio_device
{
    // the I/O port BAR of the PCI device (legacy virtio devices only)
    ULONG_PTR addr;

    // true if the device uses MSI interrupts
    bool msix_used;

    // true if the VIRTIO_RING_F_EVENT_IDX feature flag has been negotiated
    bool event_suppression_enabled;

    // true if the VIRTIO_F_RING_PACKED feature flag has been negotiated
    bool packed_ring;

    // internal device operations, implemented separately for legacy and modern
    const struct virtio_device_ops *device;

    // external callbacks implemented separately by different driver model drivers
    const struct virtio_system_ops *system;

    // opaque context value passed as first argument to virtio_system_ops callbacks
    void *DeviceContext;

    // the ISR status field, reading causes the device to de-assert an interrupt
    volatile u8 *isr;

    // modern virtio device capabilities and related state
    volatile struct virtio_pci_common_cfg *common;
    volatile unsigned char *config;
    volatile unsigned char *notify_base;
    int notify_map_cap;
    u32 notify_offset_multiplier;

    size_t config_len;
    size_t notify_len;

    // maximum number of virtqueues that fit in the memory block pointed to by info
    ULONG maxQueues;

    // points to inline_info if not more than MAX_QUEUES_PER_DEVICE_DEFAULT queues
    // are used, or to an external allocation otherwise
    VirtIOQueueInfo *info;
    VirtIOQueueInfo inline_info[MAX_QUEUES_PER_DEVICE_DEFAULT];
};

/* Driver API: device init and shutdown
 * DeviceContext is a driver defined opaque value which will be passed to driver
 * supplied callbacks described in pSystemOps. pSystemOps must be non-NULL and all
 * its fields must be non-NULL. msix_used is true if and only if the device is
 * configured with MSI support.
 */
NTSTATUS virtio_device_initialize(VirtIODevice *vdev,
                                  const VirtIOSystemOps *pSystemOps,
                                  void *DeviceContext,
                                  bool msix_used);
void virtio_device_shutdown(VirtIODevice *vdev);

/* Driver API: device status manipulation
 * virtio_set_status should not be called by new drivers. Device status should only
 * be getting its bits set with virtio_add_status and reset all back to 0 with
 * virtio_device_reset. virtio_device_ready is a special version of virtio_add_status
 * which adds the VIRTIO_CONFIG_S_DRIVER_OK status bit.
 */
u8 virtio_get_status(VirtIODevice *vdev);
void virtio_set_status(VirtIODevice *vdev, u8 status);
void virtio_add_status(VirtIODevice *vdev, u8 status);

void virtio_device_reset(VirtIODevice *vdev);
void virtio_device_ready(VirtIODevice *vdev);

/* Driver API: device feature bitmap manipulation
 * Features passed to virtio_set_features should be a subset of features offered by
 * the device as returned from virtio_get_features. virtio_set_features sets the
 * VIRTIO_CONFIG_S_FEATURES_OK status bit if it is supported by the device.
 */
#define virtio_is_feature_enabled(FeaturesList, Feature)  (!!((FeaturesList) & (1ULL << (Feature))))
#define virtio_feature_enable(FeaturesList, Feature)      ((FeaturesList) |= (1ULL << (Feature)))
#define virtio_feature_disable(FeaturesList, Feature)     ((FeaturesList) &= ~(1ULL << (Feature)))

u64 virtio_get_features(VirtIODevice *dev);
NTSTATUS virtio_set_features(VirtIODevice *vdev, u64 features);

/* Driver API: device configuration access
 * Both virtio_get_config and virtio_set_config support arbitrary values of the len
 * parameter. Config items of length 1, 2, and 4 are read/written using one access,
 * length 8 is broken down to two 4 bytes accesses, and any other length is read or
 * written byte by byte.
 */
void virtio_get_config(VirtIODevice *vdev, unsigned offset,
                       void *buf, unsigned len);
void virtio_set_config(VirtIODevice *vdev, unsigned offset,
                       void *buf, unsigned len);

/* Driver API: virtqueue setup
 * virtio_reserve_queue_memory makes VirtioLib reserve memory for its virtqueue
 * bookkeeping. Drivers should call this function if they intend to set up queues
 * one by one with virtio_find_queue. virtio_find_queues (plural) internally takes
 * care of the reservation and virtio_reserve_queue_memory need not be called.
 * Note that in addition to queue interrupt vectors, virtio_find_queues also sets
 * up the device config vector as a convenience.
 * Drivers should treat the returned struct virtqueue pointers as opaque handles.
 */
NTSTATUS virtio_query_queue_allocation(VirtIODevice *vdev, unsigned index,
                                       unsigned short *pNumEntries,
                                       unsigned long *pRingSize,
                                       unsigned long *pHeapSize);

NTSTATUS virtio_reserve_queue_memory(VirtIODevice *vdev, unsigned nvqs);

NTSTATUS virtio_find_queue(VirtIODevice *vdev, unsigned index,
                           struct virtqueue **vq);
NTSTATUS virtio_find_queues(VirtIODevice *vdev, unsigned nvqs,
                            struct virtqueue *vqs[]);

/* Driver API: virtqueue shutdown
 * The device must be reset and re-initialized to re-setup queues after they have
 * been deleted.
 */
void virtio_delete_queue(struct virtqueue *vq);
void virtio_delete_queues(VirtIODevice *vdev);

/* Driver API: virtqueue query and manipulation
 * virtio_get_queue_descriptor_size
 * is useful in situations where the driver has to prepare for the memory allocation
 * performed by virtio_reserve_queue_memory beforehand.
 */

u32 virtio_get_queue_size(struct virtqueue *vq);
unsigned long virtio_get_indirect_page_capacity();

static ULONG FORCEINLINE virtio_get_queue_descriptor_size()
{
    return sizeof(VirtIOQueueInfo);
}

/* Driver API: interrupt handling
 * virtio_set_config_vector and virtio_set_queue_vector set the MSI vector used for
 * device configuration interrupt and queue interrupt, respectively. The driver may
 * choose to either return the vector from the vdev_get_msix_vector callback (called
 * as part of queue setup) or call these functions later. Note that setting the vector
 * may fail which is indicated by the return value of VIRTIO_MSI_NO_VECTOR.
 * virtio_read_isr_status returns the value of the ISR status register, note that it
 * is not idempotent, calling the function makes the device de-assert the interrupt.
 */
u16 virtio_set_config_vector(VirtIODevice *vdev, u16 vector);
u16 virtio_set_queue_vector(struct virtqueue *vq, u16 vector);

u8 virtio_read_isr_status(VirtIODevice *vdev);

/* Driver API: miscellaneous helpers
 * virtio_get_bar_index returns the corresponding BAR index given its physical address.
 * This tends to be useful to all drivers since Windows doesn't provide reliable BAR
 * indices as part of resource enumeration. The function returns -1 on failure.
 */
int virtio_get_bar_index(PPCI_COMMON_HEADER pPCIHeader, PHYSICAL_ADDRESS BasePA);

#endif
