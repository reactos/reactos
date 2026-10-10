/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Common declarations for usbhub3
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbioctl.h>

#define HUB_POOL_TAG '3buH'

/** One per hub FDO, root hub or external. */
typedef struct _HUB_FDO_CONTEXT
{
    WDFDEVICE Device;
    WDFIOTARGET ParentTarget;
} HUB_FDO_CONTEXT, *PHUB_FDO_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HUB_FDO_CONTEXT, HubGetFdoContext);

/* driver.cpp */
extern "C" DRIVER_INITIALIZE DriverEntry;

/* hub.cpp */
EVT_WDF_DRIVER_DEVICE_ADD HubEvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE HubEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE HubEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY HubEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT HubEvtDeviceD0Exit;
