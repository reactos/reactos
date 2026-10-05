/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Common declarations for usbxhci
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <ucxclass.h>
#include <ucxstub.h>

#define XHCI_POOL_TAG 'ichX'

typedef struct _XHCI_CONTROLLER
{
    WDFDEVICE Device;
    UCXCONTROLLER UcxController;
    UCXROOTHUB UcxRootHub;
} XHCI_CONTROLLER, *PXHCI_CONTROLLER;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_CONTROLLER, XhciGetController);

/** Lets UCX callbacks get from their controller handle back to the device. */
typedef struct _XHCI_UCX_CONTROLLER_CONTEXT
{
    PXHCI_CONTROLLER Controller;
} XHCI_UCX_CONTROLLER_CONTEXT, *PXHCI_UCX_CONTROLLER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_UCX_CONTROLLER_CONTEXT, XhciGetUcxControllerContext);

/* driver.cpp */
extern "C" DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD XhciEvtDriverUnload;

/* controller.cpp */
EVT_WDF_DRIVER_DEVICE_ADD XhciEvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE XhciEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE XhciEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY XhciEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT XhciEvtDeviceD0Exit;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL XhciEvtIoDeviceControl;
EVT_UCX_CONTROLLER_QUERY_USB_CAPABILITY XhciEvtControllerQueryUsbCapability;
EVT_UCX_CONTROLLER_GET_CURRENT_FRAMENUMBER XhciEvtControllerGetCurrentFrameNumber;
EVT_UCX_CONTROLLER_RESET XhciEvtControllerReset;

/* roothub.cpp */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
XhciRootHubCreate(
    _In_ PXHCI_CONTROLLER Controller);

EVT_UCX_ROOTHUB_CONTROL_URB XhciEvtRootHubClearHubFeature;
EVT_UCX_ROOTHUB_CONTROL_URB XhciEvtRootHubClearPortFeature;
EVT_UCX_ROOTHUB_CONTROL_URB XhciEvtRootHubGetHubStatus;
EVT_UCX_ROOTHUB_CONTROL_URB XhciEvtRootHubGetPortStatus;
EVT_UCX_ROOTHUB_CONTROL_URB XhciEvtRootHubSetHubFeature;
EVT_UCX_ROOTHUB_CONTROL_URB XhciEvtRootHubSetPortFeature;
EVT_UCX_ROOTHUB_CONTROL_URB XhciEvtRootHubGetPortErrorCount;
EVT_UCX_ROOTHUB_INTERRUPT_TX XhciEvtRootHubInterruptTx;
EVT_UCX_ROOTHUB_GET_INFO XhciEvtRootHubGetInfo;
EVT_UCX_ROOTHUB_GET_20PORT_INFO XhciEvtRootHubGet20PortInfo;
EVT_UCX_ROOTHUB_GET_30PORT_INFO XhciEvtRootHubGet30PortInfo;

/* usbdevice.cpp */
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
