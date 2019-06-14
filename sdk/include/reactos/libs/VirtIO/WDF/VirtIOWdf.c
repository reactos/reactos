/*
 * Implementation of VirtioLib-WDF driver API
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

#include <wdmguid.h>

extern VirtIOSystemOps VirtIOWdfSystemOps;

NTSTATUS VirtIOWdfInitialize(PVIRTIO_WDF_DRIVER pWdfDriver,
                             WDFDEVICE Device,
                             WDFCMRESLIST ResourcesTranslated,
                             WDFINTERRUPT ConfigInterrupt,
                             ULONG MemoryTag)
{
    NTSTATUS status = STATUS_SUCCESS;

    RtlZeroMemory(pWdfDriver, sizeof(*pWdfDriver));
    pWdfDriver->MemoryTag = MemoryTag;

    /* get the PCI bus interface */
    status = WdfFdoQueryForInterface(
        Device,
        &GUID_BUS_INTERFACE_STANDARD,
        (PINTERFACE)&pWdfDriver->PCIBus,
        sizeof(pWdfDriver->PCIBus),
        1 /* version */,
        NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* register config interrupt */
    status = PCIRegisterInterrupt(ConfigInterrupt);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* set up resources */
    status = PCIAllocBars(ResourcesTranslated, pWdfDriver);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* initialize the underlying VirtIODevice */
    status = virtio_device_initialize(
        &pWdfDriver->VIODevice,
        &VirtIOWdfSystemOps,
        pWdfDriver,
        pWdfDriver->nMSIInterrupts > 0);
    if (!NT_SUCCESS(status)) {
        PCIFreeBars(pWdfDriver);
    }

    pWdfDriver->ConfigInterrupt = ConfigInterrupt;

    return status;
}

ULONGLONG VirtIOWdfGetDeviceFeatures(PVIRTIO_WDF_DRIVER pWdfDriver)
{
    return virtio_get_features(&pWdfDriver->VIODevice);
}

NTSTATUS VirtIOWdfSetDriverFeatures(PVIRTIO_WDF_DRIVER pWdfDriver,
                                    ULONGLONG uFeatures)
{
    /* make sure that we always follow the status bit-setting protocol */
    u8 status = virtio_get_status(&pWdfDriver->VIODevice);
    if (!(status & VIRTIO_CONFIG_S_ACKNOWLEDGE)) {
        virtio_add_status(&pWdfDriver->VIODevice, VIRTIO_CONFIG_S_ACKNOWLEDGE);
    }
    if (!(status & VIRTIO_CONFIG_S_DRIVER)) {
        virtio_add_status(&pWdfDriver->VIODevice, VIRTIO_CONFIG_S_DRIVER);
    }

    /* cache driver features in case we need to replay this in VirtIOWdfInitQueues */
    pWdfDriver->uFeatures = uFeatures;
    return virtio_set_features(&pWdfDriver->VIODevice, uFeatures);
}

static NTSTATUS VirtIOWdfFinalizeFeatures(PVIRTIO_WDF_DRIVER pWdfDriver)
{
    NTSTATUS status = STATUS_SUCCESS;

    u8 dev_status = virtio_get_status(&pWdfDriver->VIODevice);
    if (!(dev_status & VIRTIO_CONFIG_S_ACKNOWLEDGE)) {
        virtio_add_status(&pWdfDriver->VIODevice, VIRTIO_CONFIG_S_ACKNOWLEDGE);
    }
    if (!(dev_status & VIRTIO_CONFIG_S_DRIVER)) {
        virtio_add_status(&pWdfDriver->VIODevice, VIRTIO_CONFIG_S_DRIVER);
    }
    if (!(dev_status & VIRTIO_CONFIG_S_FEATURES_OK)) {
        status = virtio_set_features(&pWdfDriver->VIODevice, pWdfDriver->uFeatures);
    }

    return status;
}

NTSTATUS VirtIOWdfInitQueues(PVIRTIO_WDF_DRIVER pWdfDriver,
                             ULONG nQueues,
                             struct virtqueue **pQueues,
                             PVIRTIO_WDF_QUEUE_PARAM pQueueParams)
{
    NTSTATUS status;
    ULONG i;

    /* make sure that we always follow the status bit-setting protocol */
    status = VirtIOWdfFinalizeFeatures(pWdfDriver);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* register queue interrupts */
    for (i = 0; i < nQueues; i++) {
        status = PCIRegisterInterrupt(pQueueParams[i].Interrupt);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    /* find and initialize queues */
    pWdfDriver->pQueueParams = pQueueParams;
    status = virtio_find_queues(
        &pWdfDriver->VIODevice,
        nQueues,
        pQueues);
    pWdfDriver->pQueueParams = NULL;

    return status;
}

NTSTATUS VirtIOWdfInitQueuesCB(PVIRTIO_WDF_DRIVER pWdfDriver,
                               ULONG nQueues,
                               VirtIOWdfGetQueueParamCallback pQueueParamFunc,
                               VirtIOWdfSetQueueCallback pSetQueueFunc)
{
    VIRTIO_WDF_QUEUE_PARAM QueueParam;
    struct virtqueue *vq;
    NTSTATUS status;
    u16 msix_vec;
    ULONG i;

    /* make sure that we always follow the status bit-setting protocol */
    status = VirtIOWdfFinalizeFeatures(pWdfDriver);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* let VirtioLib know how many queues we'll need */
    status = virtio_reserve_queue_memory(&pWdfDriver->VIODevice, nQueues);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* set up the device config vector */
    msix_vec = PCIGetMSIInterruptVector(pWdfDriver->ConfigInterrupt);
    if (msix_vec != VIRTIO_MSI_NO_VECTOR) {
        if (virtio_set_config_vector(&pWdfDriver->VIODevice, msix_vec) != msix_vec) {
            return STATUS_DEVICE_BUSY;
        }
    }

    /* find and initialize queues */
    for (i = 0; i < nQueues; i++) {
        status = virtio_find_queue(&pWdfDriver->VIODevice, i, &vq);
        if (!NT_SUCCESS(status)) {
            break;
        }

        /* set the desired queue vector */
        QueueParam.Interrupt = NULL;

        pQueueParamFunc(pWdfDriver, i, &QueueParam);

        status = PCIRegisterInterrupt(QueueParam.Interrupt);
        if (!NT_SUCCESS(status)) {
            break;
        }

        msix_vec = PCIGetMSIInterruptVector(QueueParam.Interrupt);
        if (msix_vec != VIRTIO_MSI_NO_VECTOR) {
            if (virtio_set_queue_vector(vq, msix_vec) != msix_vec) {
                status = STATUS_DEVICE_BUSY;
                break;
            }
        }

        /* pass the virtqueue pointer to the caller */
        pSetQueueFunc(pWdfDriver, i, vq);
    }

    if (!NT_SUCCESS(status)) {
        virtio_delete_queues(&pWdfDriver->VIODevice);
    }
    return status;
}

void VirtIOWdfSetDriverOK(PVIRTIO_WDF_DRIVER pWdfDriver)
{
    virtio_device_ready(&pWdfDriver->VIODevice);
}

void VirtIOWdfSetDriverFailed(PVIRTIO_WDF_DRIVER pWdfDriver)
{
    virtio_add_status(&pWdfDriver->VIODevice, VIRTIO_CONFIG_S_FAILED);
}

NTSTATUS VirtIOWdfShutdown(PVIRTIO_WDF_DRIVER pWdfDriver)
{
    virtio_device_shutdown(&pWdfDriver->VIODevice);

    PCIFreeBars(pWdfDriver);

    return STATUS_SUCCESS;
}

NTSTATUS VirtIOWdfDestroyQueues(PVIRTIO_WDF_DRIVER pWdfDriver)
{
    virtio_device_reset(&pWdfDriver->VIODevice);
    virtio_delete_queues(&pWdfDriver->VIODevice);

    return STATUS_SUCCESS;
}

void VirtIOWdfDeviceGet(PVIRTIO_WDF_DRIVER pWdfDriver,
                        ULONG offset,
                        PVOID buf,
                        ULONG len)
{
    virtio_get_config(
        &pWdfDriver->VIODevice,
        offset,
        buf,
        len);
}

void VirtIOWdfDeviceSet(PVIRTIO_WDF_DRIVER pWdfDriver,
                        ULONG offset,
                        CONST PVOID buf,
                        ULONG len)
{
    virtio_set_config(
        &pWdfDriver->VIODevice,
        offset,
        buf,
        len);
}

UCHAR VirtIOWdfGetISRStatus(PVIRTIO_WDF_DRIVER pWdfDriver)
{
    return virtio_read_isr_status(&pWdfDriver->VIODevice);
}
