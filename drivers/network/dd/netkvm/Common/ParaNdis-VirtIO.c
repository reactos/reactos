/*
 * This file contains NDIS driver VirtIO callbacks
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
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
#include "ndis56common.h"

/////////////////////////////////////////////////////////////////////////////////////
//
// ReadVirtIODeviceRegister\WriteVirtIODeviceRegister
// NDIS specific implementation of the IO and memory space read\write
//
// The lower 64k of memory is never mapped so we can use the same routines
// for both port I/O and memory access and use the address alone to decide
// which space to use.
/////////////////////////////////////////////////////////////////////////////////////

#define PORT_MASK 0xFFFF

static u32 ReadVirtIODeviceRegister(ULONG_PTR ulRegister)
{
    ULONG ulValue;

    if (ulRegister & ~PORT_MASK) {
        NdisReadRegisterUlong(ulRegister, &ulValue);
    } else {
        NdisRawReadPortUlong(ulRegister, &ulValue);
    }

    DPrintf(6, ("[%s]R[%x]=%x", __FUNCTION__, (ULONG)ulRegister, ulValue));
    return ulValue;
}

static void WriteVirtIODeviceRegister(ULONG_PTR ulRegister, u32 ulValue)
{
    DPrintf(6, ("[%s]R[%x]=%x", __FUNCTION__, (ULONG)ulRegister, ulValue));

    if (ulRegister & ~PORT_MASK) {
        NdisWriteRegisterUlong((PULONG)ulRegister, ulValue);
    } else {
        NdisRawWritePortUlong(ulRegister, ulValue);
    }
}

static u8 ReadVirtIODeviceByte(ULONG_PTR ulRegister)
{
    u8 bValue;

    if (ulRegister & ~PORT_MASK) {
        NdisReadRegisterUchar(ulRegister, &bValue);
    } else {
        NdisRawReadPortUchar(ulRegister, &bValue);
    }

    DPrintf(6, ("[%s]R[%x]=%x", __FUNCTION__, (ULONG)ulRegister, bValue));
    return bValue;
}

static void WriteVirtIODeviceByte(ULONG_PTR ulRegister, u8 bValue)
{
    DPrintf(6, ("[%s]R[%x]=%x", __FUNCTION__, (ULONG)ulRegister, bValue));

    if (ulRegister & ~PORT_MASK) {
        NdisWriteRegisterUchar((PUCHAR)ulRegister, bValue);
    } else {
        NdisRawWritePortUchar(ulRegister, bValue);
    }
}

static u16 ReadVirtIODeviceWord(ULONG_PTR ulRegister)
{
    u16 wValue;

    if (ulRegister & ~PORT_MASK) {
        NdisReadRegisterUshort(ulRegister, &wValue);
    } else {
        NdisRawReadPortUshort(ulRegister, &wValue);
    }

    DPrintf(6, ("[%s]R[%x]=%x\n", __FUNCTION__, (ULONG)ulRegister, wValue));
    return wValue;
}

static void WriteVirtIODeviceWord(ULONG_PTR ulRegister, u16 wValue)
{
#if 1
    if (ulRegister & ~PORT_MASK) {
        NdisWriteRegisterUshort((PUSHORT)ulRegister, wValue);
    } else {
        NdisRawWritePortUshort(ulRegister, wValue);
    }
#else
    // test only to cause long TX waiting queue of NDIS packets
    // to recognize it and request for reset via Hang handler
    static int nCounterToFail = 0;
    static const int StartFail = 200, StopFail = 600;
    BOOLEAN bFail = FALSE;
    DPrintf(6, ("%s> R[%x] = %x\n", __FUNCTION__, (ULONG)ulRegister, wValue));
    if ((ulRegister & 0x1F) == 0x10)
    {
        nCounterToFail++;
        bFail = nCounterToFail >= StartFail && nCounterToFail < StopFail;
    }
    if (!bFail) NdisRawWritePortUshort(ulRegister, wValue);
    else
    {
        DPrintf(0, ("%s> FAILING R[%x] = %x\n", __FUNCTION__, (ULONG)ulRegister, wValue));
    }
#endif
}

static void *mem_alloc_contiguous_pages(void *context, size_t size)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)context;
    PVOID retVal = NULL;
    ULONG i;

    /* find the first unused memory range of the requested size */
    for (i = 0; i < MAX_NUM_OF_QUEUES; i++) {
        if (pContext->SharedMemoryRanges[i].pBase != NULL &&
            pContext->SharedMemoryRanges[i].bUsed == FALSE &&
            pContext->SharedMemoryRanges[i].uLength == (ULONG)size) {
            retVal = pContext->SharedMemoryRanges[i].pBase;
            pContext->SharedMemoryRanges[i].bUsed = TRUE;
            break;
        }
    }

    if (!retVal) {
        /* find the first null memory range descriptor and allocate */
        for (i = 0; i < MAX_NUM_OF_QUEUES; i++) {
            if (pContext->SharedMemoryRanges[i].pBase == NULL) {
                break;
            }
        }
        if (i < MAX_NUM_OF_QUEUES) {
            NdisMAllocateSharedMemory(
                pContext->MiniportHandle,
                (ULONG)size,
                TRUE /* Cached */,
                &pContext->SharedMemoryRanges[i].pBase,
                &pContext->SharedMemoryRanges[i].BasePA);
            retVal = pContext->SharedMemoryRanges[i].pBase;
            if (retVal) {
                NdisZeroMemory(retVal, size);
                pContext->SharedMemoryRanges[i].uLength = (ULONG)size;
                pContext->SharedMemoryRanges[i].bUsed = TRUE;
            }
        }
    }

    if (retVal) {
        DPrintf(6, ("[%s] returning %p, size %x\n", __FUNCTION__, retVal, (ULONG)size));
    } else {
        DPrintf(0, ("[%s] failed to allocate size %x\n", __FUNCTION__, (ULONG)size));
    }
    return retVal;
}

static void mem_free_contiguous_pages(void *context, void *virt)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)context;
    ULONG i;

    for (i = 0; i < MAX_NUM_OF_QUEUES; i++) {
        if (pContext->SharedMemoryRanges[i].pBase == virt) {
            pContext->SharedMemoryRanges[i].bUsed = FALSE;
            break;
        }
    }

    if (i < MAX_NUM_OF_QUEUES) {
        DPrintf(6, ("[%s] freed %p at index %d\n", __FUNCTION__, virt, i));
    } else {
        DPrintf(0, ("[%s] failed to free %p\n", __FUNCTION__, virt));
    }
}

static ULONGLONG mem_get_physical_address(void *context, void *virt)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)context;
    ULONG_PTR uAddr = (ULONG_PTR)virt;
    ULONG i;

    for (i = 0; i < MAX_NUM_OF_QUEUES; i++) {
        ULONG_PTR uBase = (ULONG_PTR)pContext->SharedMemoryRanges[i].pBase;
        if (uAddr >= uBase && uAddr < (uBase + pContext->SharedMemoryRanges[i].uLength)) {
            ULONGLONG retVal = pContext->SharedMemoryRanges[i].BasePA.QuadPart + (uAddr - uBase);

            DPrintf(6, ("[%s] translated %p to %I64X\n", __FUNCTION__, virt, retVal));
            return retVal;
        }
    }

    DPrintf(0, ("[%s] failed to translate %p\n", __FUNCTION__, virt));
    return 0;
}

static void *mem_alloc_nonpaged_block(void *context, size_t size)
{
    PVOID retVal;

    if (NdisAllocateMemoryWithTag(
        &retVal,
        (UINT)size,
        PARANDIS_MEMORY_TAG) != NDIS_STATUS_SUCCESS) {
        retVal = NULL;
    }

    if (retVal) {
        NdisZeroMemory(retVal, size);
        DPrintf(6, ("[%s] returning %p, len %x\n", __FUNCTION__, retVal, (ULONG)size));
    } else {
        DPrintf(0, ("[%s] failed to allocate size %x\n", __FUNCTION__, (ULONG)size));
    }
    return retVal;
}

static void mem_free_nonpaged_block(void *context, void *addr)
{
    UNREFERENCED_PARAMETER(context);

    NdisFreeMemory(addr, 0, 0);
    DPrintf(6, ("[%s] freed %p\n", __FUNCTION__, addr));
}

static int PCIReadConfig(PPARANDIS_ADAPTER pContext,
                         int where,
                         void *buffer,
                         size_t length)
{
    ULONG read;

    read = NdisReadPciSlotInformation(
        pContext->MiniportHandle,
        0 /* SlotNumber */,
        where,
        buffer,
        (ULONG)length);

    if (read == length) {
        DPrintf(6, ("[%s] read %d bytes at %d\n", __FUNCTION__, read, where));
        return 0;
    } else {
        DPrintf(0, ("[%s] failed to read %d bytes at %d\n", __FUNCTION__, read, where));
        return -1;
    }
}

static int pci_read_config_byte(void *context, int where, u8 *bVal)
{
    return PCIReadConfig((PPARANDIS_ADAPTER)context, where, bVal, sizeof(*bVal));
}

static int pci_read_config_word(void *context, int where, u16 *wVal)
{
    return PCIReadConfig((PPARANDIS_ADAPTER)context, where, wVal, sizeof(*wVal));
}

static int pci_read_config_dword(void *context, int where, u32 *dwVal)
{
    return PCIReadConfig((PPARANDIS_ADAPTER)context, where, dwVal, sizeof(*dwVal));
}

static size_t pci_get_resource_len(void *context, int bar)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)context;

    if (bar < PCI_TYPE0_ADDRESSES) {
        return pContext->AdapterResources.PciBars[bar].uLength;
    }

    DPrintf(0, ("[%s] queried invalid BAR %d\n", __FUNCTION__, bar));
    return 0;
}

static void *pci_map_address_range(void *context, int bar, size_t offset, size_t maxlen)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)context;

    if (bar < PCI_TYPE0_ADDRESSES) {
        tBusResource *pRes = &pContext->AdapterResources.PciBars[bar];
        if (pRes->pBase == NULL) {
            /* BAR not mapped yet */
            if (pRes->bPortSpace) {
                if (NDIS_STATUS_SUCCESS == NdisMRegisterIoPortRange(
                    &pRes->pBase,
                    pContext->MiniportHandle,
                    pRes->BasePA.LowPart,
                    pRes->uLength)) {
                    DPrintf(6, ("[%s] mapped port BAR at %x\n", __FUNCTION__, pRes->BasePA.LowPart));
                } else {
                    pRes->pBase = NULL;
                    DPrintf(0, ("[%s] failed to map port BAR at %x\n", __FUNCTION__, pRes->BasePA.LowPart));
                }
            } else {
                if (NDIS_STATUS_SUCCESS == NdisMMapIoSpace(
                    &pRes->pBase,
                    pContext->MiniportHandle,
                    pRes->BasePA,
                    pRes->uLength)) {
                    DPrintf(6, ("[%s] mapped memory BAR at %I64x\n", __FUNCTION__, pRes->BasePA.QuadPart));
                } else {
                    pRes->pBase = NULL;
                    DPrintf(0, ("[%s] failed to map memory BAR at %I64x\n", __FUNCTION__, pRes->BasePA.QuadPart));
                }
            }
        }
        if (pRes->pBase != NULL && offset < pRes->uLength) {
            if (pRes->bPortSpace) {
                /* use physical address for port I/O */
                return (PUCHAR)(ULONG_PTR)pRes->BasePA.LowPart + offset;
            } else {
                /* use virtual address for memory I/O */
                return (PUCHAR)pRes->pBase + offset;
            }
        } else {
            DPrintf(0, ("[%s] failed to get map BAR %d, offset %x\n", __FUNCTION__, bar, offset));
        }
    } else {
        DPrintf(0, ("[%s] queried invalid BAR %d\n", __FUNCTION__, bar));
    }

    return NULL;
}

static u16 vdev_get_msix_vector(void *context, int queue)
{
    PARANDIS_ADAPTER *pContext = (PARANDIS_ADAPTER *)context;
    u16 vector = VIRTIO_MSI_NO_VECTOR;

    /* we don't run on MSI support so this will never be true */
    if (pContext->bUsingMSIX && queue >= 0) {
        vector = (u16)pContext->AdapterResources.Vector;
    }

    return vector;
}

static void vdev_sleep(void *context, unsigned int msecs)
{
    UNREFERENCED_PARAMETER(context);

    NdisMSleep(1000 * msecs);
}

VirtIOSystemOps ParaNdisSystemOps = {
    /* .vdev_read_byte = */ ReadVirtIODeviceByte,
    /* .vdev_read_word = */ ReadVirtIODeviceWord,
    /* .vdev_read_dword = */ ReadVirtIODeviceRegister,
    /* .vdev_write_byte = */ WriteVirtIODeviceByte,
    /* .vdev_write_word = */ WriteVirtIODeviceWord,
    /* .vdev_write_dword = */ WriteVirtIODeviceRegister,
    /* .mem_alloc_contiguous_pages = */ mem_alloc_contiguous_pages,
    /* .mem_free_contiguous_pages = */ mem_free_contiguous_pages,
    /* .mem_get_physical_address = */ mem_get_physical_address,
    /* .mem_alloc_nonpaged_block = */ mem_alloc_nonpaged_block,
    /* .mem_free_nonpaged_block = */ mem_free_nonpaged_block,
    /* .pci_read_config_byte = */ pci_read_config_byte,
    /* .pci_read_config_word = */ pci_read_config_word,
    /* .pci_read_config_dword = */ pci_read_config_dword,
    /* .pci_get_resource_len = */ pci_get_resource_len,
    /* .pci_map_address_range = */ pci_map_address_range,
    /* .vdev_get_msix_vector = */ vdev_get_msix_vector,
    /*.vdev_sleep = */ vdev_sleep,
};
