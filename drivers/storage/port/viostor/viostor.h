/*
 * Reduced single-queue legacy VirtIO-blk Storport miniport.
 *
 * This driver uses the ReactOS VirtIO library and the legacy PCI transport
 * (PCI device 1AF4:1001). Modern transport, MSI-X and multi-queue are
 * deliberately not advertised by this implementation.
 *
 * Copyright (c) 2026 ReactOS contributors.
 *
 * The VirtIO wire-format definitions in this file are derived from the
 * BSD-3-Clause virtio-win viostor source at commit
 * a66c7af4aef5ded62b2b19048df7e9c396b927ef.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _REACTOS_VIOSTOR_H_
#define _REACTOS_VIOSTOR_H_

#include <ntddk.h>
#include <storport.h>
#include <ntddscsi.h>
#include <virtio_pci.h>
#include <VirtIO.h>

#define VIOSTOR_VENDOR_ID              0x1AF4
#define VIOSTOR_DEVICE_ID              0x1001
#define VIOSTOR_SECTOR_SIZE            512u
#define VIOSTOR_QUEUE_INDEX             0u
#define VIOSTOR_QUEUE_MEMORY_SIZE       (16u * PAGE_SIZE)
#define VIOSTOR_MAX_SG                  32u
#define VIOSTOR_MAX_TRANSFER_LENGTH     ((VIOSTOR_MAX_SG - 2u) * PAGE_SIZE)
#define VIOSTOR_POOL_TAG                'tSiv'

#define VIRTIO_BLK_F_BARRIER             0
#define VIRTIO_BLK_F_SIZE_MAX            1
#define VIRTIO_BLK_F_SEG_MAX             2
#define VIRTIO_BLK_F_GEOMETRY            4
#define VIRTIO_BLK_F_RO                  5
#define VIRTIO_BLK_F_BLK_SIZE            6
#define VIRTIO_BLK_F_SCSI                7
#define VIRTIO_BLK_F_FLUSH               9
#define VIRTIO_BLK_F_TOPOLOGY           10
#define VIRTIO_BLK_F_CONFIG_WCE         11
#define VIRTIO_BLK_F_MQ                12
#define VIRTIO_BLK_F_DISCARD           13
#define VIRTIO_BLK_F_WRITE_ZEROES      14

#define VIRTIO_BLK_T_IN                  0
#define VIRTIO_BLK_T_OUT                 1
#define VIRTIO_BLK_T_FLUSH               4
#define VIRTIO_BLK_S_OK                  0
#define VIRTIO_BLK_S_IOERR               1
#define VIRTIO_BLK_S_UNSUPP              2

#pragma pack(push, 1)
typedef struct _VIOSTOR_BLK_CONFIG {
    ULONGLONG Capacity;
    ULONG SizeMax;
    ULONG SegMax;
    USHORT Cylinders;
    UCHAR Heads;
    UCHAR Sectors;
    ULONG BlockSize;
} VIOSTOR_BLK_CONFIG, *PVIOSTOR_BLK_CONFIG;

typedef struct _VIOSTOR_BLK_OUT_HEADER {
    ULONG Type;
    ULONG IoPriority;
    ULONGLONG Sector;
} VIOSTOR_BLK_OUT_HEADER, *PVIOSTOR_BLK_OUT_HEADER;
#pragma pack(pop)

typedef struct _VIOSTOR_SRB_EXTENSION {
    PSCSI_REQUEST_BLOCK Srb;
    VIOSTOR_BLK_OUT_HEADER Header;
    UCHAR Status;
    ULONG SgCount;
    ULONG TransferLength;
    PSTOR_SCATTER_GATHER_LIST ScatterGather;
    struct VirtIOBufferDescriptor Sg[VIOSTOR_MAX_SG];
} VIOSTOR_SRB_EXTENSION, *PVIOSTOR_SRB_EXTENSION;


typedef struct _VIOSTOR_ADAPTER_EXTENSION {
    VirtIODevice Device;
    struct virtqueue *Queue;
    VIOSTOR_BLK_CONFIG Config;
    ULONGLONG Features;
    ULONGLONG LastLba;
    PVOID QueueMemory;
    BOOLEAN QueueMemoryUsed;
    PVOID IoBase;
    ULONG IoLength;
    ULONG MaximumTransferLength;
    ULONG NumberOfPhysicalBreaks;
    KSPIN_LOCK Lock;
    BOOLEAN PortSpace;
    ULONG SystemIoBusNumber;
    ULONG SlotNumber;
    ULONG PciConfigLength;
    PCI_COMMON_CONFIG PciConfig;
    PPORT_CONFIGURATION_INFORMATION PortConfig;
    BOOLEAN Initialized;
} VIOSTOR_ADAPTER_EXTENSION, *PVIOSTOR_ADAPTER_EXTENSION;

DRIVER_INITIALIZE DriverEntry;

#endif
