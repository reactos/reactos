/*
 * Implementation of virtio_system_ops VirtioLib callbacks
 *
 * Copyright (c) 2016-2017 Red Hat, Inc.
 *
 * Author(s):
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
#include "VirtIOWdf.h"
#include "private.h"

static void *mem_alloc_contiguous_pages(void *context, size_t size)
{
    PHYSICAL_ADDRESS HighestAcceptable;
    void *ret;

    UNREFERENCED_PARAMETER(context);

    HighestAcceptable.QuadPart = 0xFFFFFFFFFF;
#if defined(NTDDI_WIN8) && (NTDDI_VERSION >= NTDDI_WIN8)
    {
        PHYSICAL_ADDRESS Zero = { 0 };
        ret = MmAllocateContiguousNodeMemory(
            size,
            Zero,
            HighestAcceptable,
            Zero,
            PAGE_READWRITE,
            MM_ANY_NODE_OK);
    }
#else
    ret = MmAllocateContiguousMemory(size, HighestAcceptable);
#endif
    if (ret != NULL) {
        RtlZeroMemory(ret, size);
    }
    return ret;
}

static void mem_free_contiguous_pages(void *context, void *virt)
{
    UNREFERENCED_PARAMETER(context);

    MmFreeContiguousMemory(virt);
}

static ULONGLONG mem_get_physical_address(void *context, void *virt)
{
    UNREFERENCED_PARAMETER(context);

    PHYSICAL_ADDRESS pa = MmGetPhysicalAddress(virt);
    return pa.QuadPart;
}

static void *mem_alloc_nonpaged_block(void *context, size_t size)
{
    PVIRTIO_WDF_DRIVER pWdfDriver = (PVIRTIO_WDF_DRIVER)context;

    PVOID addr = ExAllocatePoolWithTag(
        NonPagedPool,
        size,
        pWdfDriver->MemoryTag);
    if (addr) {
        RtlZeroMemory(addr, size);
    }
    return addr;
}

static void mem_free_nonpaged_block(void *context, void *addr)
{
    PVIRTIO_WDF_DRIVER pWdfDriver = (PVIRTIO_WDF_DRIVER)context;

    ExFreePoolWithTag(
        addr,
        pWdfDriver->MemoryTag);
}

static int pci_read_config_byte(void *context, int where, u8 *bVal)
{
    return PCIReadConfig((PVIRTIO_WDF_DRIVER)context, where, bVal, sizeof(*bVal));
}

static int pci_read_config_word(void *context, int where, u16 *wVal)
{
    return PCIReadConfig((PVIRTIO_WDF_DRIVER)context, where, wVal, sizeof(*wVal));
}

static int pci_read_config_dword(void *context, int where, u32 *dwVal)
{
    return PCIReadConfig((PVIRTIO_WDF_DRIVER)context, where, dwVal, sizeof(*dwVal));
}

static PVIRTIO_WDF_BAR find_bar(void *context, int bar)
{
    PVIRTIO_WDF_DRIVER pWdfDriver = (PVIRTIO_WDF_DRIVER)context;
    PSINGLE_LIST_ENTRY iter = &pWdfDriver->PCIBars;
    
    while (iter->Next != NULL) {
        PVIRTIO_WDF_BAR pBar = CONTAINING_RECORD(iter->Next, VIRTIO_WDF_BAR, ListEntry);
        if (pBar->iBar == bar) {
            return pBar;
        }
        iter = iter->Next;
    }
    return NULL;
}

static size_t pci_get_resource_len(void *context, int bar)
{
    PVIRTIO_WDF_BAR pBar = find_bar(context, bar);
    return (pBar ? pBar->uLength : 0);
}

static void *pci_map_address_range(void *context, int bar, size_t offset, size_t maxlen)
{
    PVIRTIO_WDF_BAR pBar = find_bar(context, bar);
    if (pBar) {
        if (pBar->pBase == NULL) {
            ASSERT(!pBar->bPortSpace);
#if defined(NTDDI_WINTHRESHOLD) && (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
            pBar->pBase = MmMapIoSpaceEx(
                pBar->BasePA,
                pBar->uLength,
                PAGE_READWRITE | PAGE_NOCACHE);
#else
            pBar->pBase = MmMapIoSpace(pBar->BasePA, pBar->uLength, MmNonCached);
#endif
        }
        if (pBar->pBase != NULL && offset < pBar->uLength) {
            return (char *)pBar->pBase + offset;
        }
    }
    return NULL;
}

static u16 vdev_get_msix_vector(void *context, int queue)
{
    PVIRTIO_WDF_DRIVER pWdfDriver = (PVIRTIO_WDF_DRIVER)context;
    u16 vector = VIRTIO_MSI_NO_VECTOR;

    if (queue >= 0) {
        /* queue interrupt */
        if (pWdfDriver->pQueueParams != NULL) {
            vector = PCIGetMSIInterruptVector(pWdfDriver->pQueueParams[queue].Interrupt);
        }
    }
    else {
        /* on-device-config-change interrupt */
        vector = PCIGetMSIInterruptVector(pWdfDriver->ConfigInterrupt);
    }

    return vector;
}

static void vdev_sleep(void *context, unsigned int msecs)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UNREFERENCED_PARAMETER(context);

    if (KeGetCurrentIrql() <= APC_LEVEL) {
        LARGE_INTEGER delay;
        delay.QuadPart = Int32x32To64(msecs, -10000);
        status = KeDelayExecutionThread(KernelMode, FALSE, &delay);
    }

    if (!NT_SUCCESS(status)) {
        /* fall back to busy wait if we're not allowed to sleep */
        KeStallExecutionProcessor(1000 * msecs);
    }
}

extern u32 ReadVirtIODeviceRegister(ULONG_PTR ulRegister);
extern void WriteVirtIODeviceRegister(ULONG_PTR ulRegister, u32 ulValue);
extern u8 ReadVirtIODeviceByte(ULONG_PTR ulRegister);
extern void WriteVirtIODeviceByte(ULONG_PTR ulRegister, u8 bValue);
extern u16 ReadVirtIODeviceWord(ULONG_PTR ulRegister);
extern void WriteVirtIODeviceWord(ULONG_PTR ulRegister, u16 bValue);

VirtIOSystemOps VirtIOWdfSystemOps = {
    .vdev_read_byte = ReadVirtIODeviceByte,
    .vdev_read_word = ReadVirtIODeviceWord,
    .vdev_read_dword = ReadVirtIODeviceRegister,
    .vdev_write_byte = WriteVirtIODeviceByte,
    .vdev_write_word = WriteVirtIODeviceWord,
    .vdev_write_dword = WriteVirtIODeviceRegister,
    .mem_alloc_contiguous_pages = mem_alloc_contiguous_pages,
    .mem_free_contiguous_pages = mem_free_contiguous_pages,
    .mem_get_physical_address = mem_get_physical_address,
    .mem_alloc_nonpaged_block = mem_alloc_nonpaged_block,
    .mem_free_nonpaged_block = mem_free_nonpaged_block,
    .pci_read_config_byte = pci_read_config_byte,
    .pci_read_config_word = pci_read_config_word,
    .pci_read_config_dword = pci_read_config_dword,
    .pci_get_resource_len = pci_get_resource_len,
    .pci_map_address_range = pci_map_address_range,
    .vdev_get_msix_vector = vdev_get_msix_vector,
    .vdev_sleep = vdev_sleep,
};
