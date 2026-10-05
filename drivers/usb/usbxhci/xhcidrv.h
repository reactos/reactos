/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Master header; every source file includes it first
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <ucxclass.h>
#include <ucxstub.h>

#include "xhciutil.h"
#include "xhcihw.h"
#include "xhcierrata.h"

#include "mmio.h"
#include "dmapool.h"
#include "slots.h"
#include "intr.h"
#include "cmdring.h"
#include "rootports.h"
#include "hc.h"
#include "xhcisvc.h"

/* xhcientry.cpp */
extern "C" DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD XhciEvtDriverUnload;

/* hc.cpp */
EVT_WDF_DRIVER_DEVICE_ADD XhciEvtDeviceAdd;

/* slotdev.cpp (stub until the device milestone) */
EVT_UCX_CONTROLLER_USBDEVICE_ADD XhciEvtControllerUsbDeviceAdd;
EVT_UCX_USBDEVICE_ENDPOINTS_CONFIGURE XhciEvtUsbDeviceEndpointsConfigure;
EVT_UCX_USBDEVICE_ENABLE XhciEvtUsbDeviceEnable;
EVT_UCX_USBDEVICE_DISABLE XhciEvtUsbDeviceDisable;
EVT_UCX_USBDEVICE_RESET XhciEvtUsbDeviceReset;
EVT_UCX_USBDEVICE_ADDRESS XhciEvtUsbDeviceAddress;
EVT_UCX_USBDEVICE_UPDATE XhciEvtUsbDeviceUpdate;
EVT_UCX_USBDEVICE_HUB_INFO XhciEvtUsbDeviceHubInfo;
EVT_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD XhciEvtUsbDeviceDefaultEndpointAdd;
EVT_UCX_USBDEVICE_ENDPOINT_ADD XhciEvtUsbDeviceEndpointAdd;
