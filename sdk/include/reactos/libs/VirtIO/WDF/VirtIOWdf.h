/*
 * Public VirtioLib-WDF prototypes (driver API)
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
#pragma once

#include <wdf.h>
#include "virtio_pci.h"

/* Configures a virtqueue, see VirtIOWdfInitQueues. */
typedef struct virtio_wdf_queue_param {
    /* interrupt associated with the queue */
    WDFINTERRUPT            Interrupt;
} VIRTIO_WDF_QUEUE_PARAM , *PVIRTIO_WDF_QUEUE_PARAM;

/* Data associated with a WDF virtio driver, usually declared as
 * a field in the driver's context structure and treated opaque.
 */
typedef struct virtio_wdf_driver {
    VirtIODevice            VIODevice;

    ULONG                   MemoryTag;
    ULONGLONG               uFeatures;

    BUS_INTERFACE_STANDARD  PCIBus;
    SINGLE_LIST_ENTRY       PCIBars;

    ULONG                   nInterrupts;
    ULONG                   nMSIInterrupts;

    WDFINTERRUPT            ConfigInterrupt;
    PVIRTIO_WDF_QUEUE_PARAM pQueueParams;
    
} VIRTIO_WDF_DRIVER, *PVIRTIO_WDF_DRIVER;

/* Queue discovery callbacks used by VirtIOWdfInitQueuesCB. */
typedef void (*VirtIOWdfGetQueueParamCallback)(PVIRTIO_WDF_DRIVER pWdfDriver,
                                               ULONG uQueueIndex,
                                               PVIRTIO_WDF_QUEUE_PARAM pQueueParam);
typedef void (*VirtIOWdfSetQueueCallback)(PVIRTIO_WDF_DRIVER pWdfDriver,
                                          ULONG uQueueIndex,
                                          struct virtqueue *pQueue);

/* Initializes the VIRTIO_WDF_DRIVER context, called from driver's
 * EvtDevicePrepareHardware callback.
 */
NTSTATUS VirtIOWdfInitialize(PVIRTIO_WDF_DRIVER pWdfDriver,
                             WDFDEVICE Device,
                             WDFCMRESLIST ResourcesTranslated,
                             WDFINTERRUPT ConfigInterrupt,
                             ULONG MemoryTag);

/* Device/driver feature negotiation routines. These can be called from
 * driver's EvtDevicePrepareHardware callback or later from its
 * EvtDeviceD0Entry callback. If the device is reset and re-initialized
 * (D0 exit, then D0 entry) and VirtIOWdfSetDriverFeatures is not called,
 * the same features are automatically negotiated.
 */
ULONGLONG VirtIOWdfGetDeviceFeatures(PVIRTIO_WDF_DRIVER pWdfDriver);
NTSTATUS VirtIOWdfSetDriverFeatures(PVIRTIO_WDF_DRIVER pWdfDriver,
                                    ULONGLONG uFeatures);

/* Queue discovery entry points. Must be called after each device reset as
 * there is no way to reinitialize or reset individual queues. The CB
 * version takes callbacks to get queue parameters and return queue pointers.
 * The regular version uses caller-allocated arrays for the same. They are
 * functionally equivalent.
 */
NTSTATUS VirtIOWdfInitQueues(PVIRTIO_WDF_DRIVER pWdfDriver,
                             ULONG nQueues,
                             struct virtqueue **pQueues,
                             PVIRTIO_WDF_QUEUE_PARAM pQueueParams);
NTSTATUS VirtIOWdfInitQueuesCB(PVIRTIO_WDF_DRIVER pWdfDriver,
                               ULONG nQueues,
                               VirtIOWdfGetQueueParamCallback pQueueParamFunc,
                               VirtIOWdfSetQueueCallback pSetQueueFunc);

/* Final signal to the device that the driver has successfully initialized
 * and is ready for device operation or that it has failed to do so.
 * It is not legal to notify the device before VirtIOWdfSetDriverOK is called.
 */
void VirtIOWdfSetDriverOK(PVIRTIO_WDF_DRIVER pWdfDriver);
void VirtIOWdfSetDriverFailed(PVIRTIO_WDF_DRIVER pWdfDriver);

/* Resets the device and destroys virtqueue data structures. To be called
 * from driver's EvtDeviceD0Exit callback.
 */
NTSTATUS VirtIOWdfDestroyQueues(PVIRTIO_WDF_DRIVER pWdfDriver);

/* Destroys the VIRTIO_WDF_DRIVER context and deallocates all resources.
 * To be called from driver's EvtDeviceReleaseHardware callback.
 */
NTSTATUS VirtIOWdfShutdown(PVIRTIO_WDF_DRIVER pWdfDriver);

/* Returns the contents of the ISR status field and acknowledges the
 * interrupt. Called from driver's ISR if traditional IRQ interrupts
 * are used.
 */
UCHAR VirtIOWdfGetISRStatus(PVIRTIO_WDF_DRIVER pWdfDriver);

/* Device config space access routines. Follow specific device documentation
 * for rules on when and how these can be called. If interrupt on device
 * config change is desired, a valid WDFINTERRUPT should be passed to
 * VirtIOWdfInitialize.
 */
void VirtIOWdfDeviceGet(PVIRTIO_WDF_DRIVER pWdfDriver,
                        ULONG offset,
                        PVOID buf,
                        ULONG len);
void VirtIOWdfDeviceSet(PVIRTIO_WDF_DRIVER pWdfDriver,
                        ULONG offset,
                        CONST PVOID buf,
                        ULONG len);
