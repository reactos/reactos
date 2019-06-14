/*
 * PCI config space & resources helper routines
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
#include "VirtIOWdf.h"
#include "private.h"

NTSTATUS PCIAllocBars(WDFCMRESLIST ResourcesTranslated,
                      PVIRTIO_WDF_DRIVER pWdfDriver)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResDescriptor;
    ULONG nInterrupts = 0, nMSIInterrupts = 0;
    int nListSize = WdfCmResourceListGetCount(ResourcesTranslated);
    int i;
    PVIRTIO_WDF_BAR pBar;
    PCI_COMMON_HEADER PCIHeader = { 0 };

    /* read the PCI config header */
    if (pWdfDriver->PCIBus.GetBusData(
        pWdfDriver->PCIBus.Context,
        PCI_WHICHSPACE_CONFIG,
        &PCIHeader,
        0,
        sizeof(PCIHeader)) != sizeof(PCIHeader)) {
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    
    for (i = 0; i < nListSize; i++) {
        pResDescriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);
        if (pResDescriptor) {
            switch (pResDescriptor->Type) {
                case CmResourceTypePort:
                case CmResourceTypeMemory:
                    pBar = (PVIRTIO_WDF_BAR)ExAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(VIRTIO_WDF_BAR),
                        pWdfDriver->MemoryTag);
                    if (pBar == NULL) {
                        /* undo what we've done so far */
                        PCIFreeBars(pWdfDriver);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    /* unfortunately WDF doesn't tell us BAR indices */
                    pBar->iBar = virtio_get_bar_index(&PCIHeader, pResDescriptor->u.Memory.Start);
                    if (pBar->iBar < 0) {
                        /* undo what we've done so far */
                        PCIFreeBars(pWdfDriver);
                        return STATUS_NOT_FOUND;
                    }

                    pBar->bPortSpace = !!(pResDescriptor->Flags & CM_RESOURCE_PORT_IO);
                    pBar->BasePA = pResDescriptor->u.Memory.Start;
                    pBar->uLength = pResDescriptor->u.Memory.Length;

                    if (pBar->bPortSpace) {
                        pBar->pBase = (PVOID)(ULONG_PTR)pBar->BasePA.QuadPart;
                    } else {
                        /* memory regions are mapped into the virtual memory space on demand */
                        pBar->pBase = NULL;
                    }
                    PushEntryList(&pWdfDriver->PCIBars, &pBar->ListEntry);
                    break;

                case CmResourceTypeInterrupt:
                    nInterrupts++;
                    if (pResDescriptor->Flags &
                        (CM_RESOURCE_INTERRUPT_LATCHED | CM_RESOURCE_INTERRUPT_MESSAGE)) {
                        nMSIInterrupts++;
                    }
                    break;
            }
        }
    }

    pWdfDriver->nInterrupts = nInterrupts;
    pWdfDriver->nMSIInterrupts = nMSIInterrupts;

    return STATUS_SUCCESS;
}

void PCIFreeBars(PVIRTIO_WDF_DRIVER pWdfDriver)
{
    PSINGLE_LIST_ENTRY iter;

    /* unmap IO space and free our BAR descriptors */
    while (iter = PopEntryList(&pWdfDriver->PCIBars)) {
        PVIRTIO_WDF_BAR pBar = CONTAINING_RECORD(iter, VIRTIO_WDF_BAR, ListEntry);
        if (pBar->pBase != NULL && !pBar->bPortSpace) {
            MmUnmapIoSpace(pBar->pBase, pBar->uLength);
        }
        ExFreePoolWithTag(pBar, pWdfDriver->MemoryTag);
    }
}

int PCIReadConfig(PVIRTIO_WDF_DRIVER pWdfDriver,
                  int where,
                  void *buffer,
                  size_t length)
{
    ULONG read;

    read = pWdfDriver->PCIBus.GetBusData(
        pWdfDriver->PCIBus.Context,
        PCI_WHICHSPACE_CONFIG,
        buffer,
        where,
        (ULONG)length);
    return (read == length ? 0 : -1);
}

NTSTATUS PCIRegisterInterrupt(WDFINTERRUPT Interrupt)
{
    PVIRTIO_WDF_INTERRUPT_CONTEXT context;
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;

    if (Interrupt == NULL) {
        status = STATUS_SUCCESS;
    } else {
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, VIRTIO_WDF_INTERRUPT_CONTEXT);
        status = WdfObjectAllocateContext(
            Interrupt,
            &attributes,
            &context);
        if (status == STATUS_OBJECT_NAME_EXISTS) {
            /* this is fine, we want to reuse the pre-existing context */
            status = STATUS_SUCCESS;
        }
    }
    return status;
}

u16 PCIGetMSIInterruptVector(WDFINTERRUPT Interrupt)
{
    WDF_INTERRUPT_INFO info;
    PVIRTIO_WDF_INTERRUPT_CONTEXT pContext;
    u16 uMessageNumber = VIRTIO_MSI_NO_VECTOR;

    if (Interrupt != NULL) {
        pContext = GetInterruptContext(Interrupt);
        if (pContext && pContext->bMessageNumberSet) {
            uMessageNumber = pContext->uMessageNumber;
        } else {
            WDF_INTERRUPT_INFO_INIT(&info);
            WdfInterruptGetInfo(Interrupt, &info);

            if (info.MessageSignaled) {
                ASSERT(info.MessageNumber < MAXUSHORT);
                uMessageNumber = (u16)info.MessageNumber;
            }

            if (pContext) {
                pContext->uMessageNumber = uMessageNumber;
                pContext->bMessageNumberSet = true;
            }
        }
    }

    return uMessageNumber;
}
