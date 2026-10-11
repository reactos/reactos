/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Services one module of the driver offers the others
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Hub numbers from usbd.sys, shown to users in the port location string */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubAllocateHubNumber(
    _Out_ PULONG HubNumber);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubReleaseHubNumber(
    _In_ ULONG HubNumber);

/* Registry module */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubReadHubRegistryValues(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUxdShutdownCleanup(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubReadUsb4HostName(
    _In_ HubFdo* Hub);

/* ACPI, connector map and WMI modules */

/** Fails only when a port cannot be bound to the USB4 host router its ACPI node names. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubAcpiReadPortAttributes(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubConnectorMapPorts(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubConnectorUnmapPorts(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWmiRegister(
    _In_ HubFdo* Hub);

/* USB4 router DROM */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubDromReportFirmwareUpdateDevice(
    _In_ HubFdo* Hub);

/* USB4 host routers */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubUsb4Initialize(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubUsb4BindPort(
    _In_ HubFdo* Hub,
    _In_ HubPort* Port,
    _In_ WDFSTRING Name);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4MapDvsecHosts(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubUsb4AssignPortMappings(
    _In_ HubFdo* Hub);

BOOLEAN
NTAPI
HubUsb4NeedsHostPower(
    _In_ HubChild* Child);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4AcquireHostPower(
    _In_ HubPort* Port);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4ReleaseHostPower(
    _In_ HubPort* Port);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4RegisterNotification(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4UnregisterNotification(
    _In_ HubFdo* Hub);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4CloseAllHosts(
    _In_ HubFdo* Hub);

/* UCX module */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubRequestReservedIo(
    _In_ HubFdo* Hub);

/* Diagnostics */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubReportPnpProblem(
    _In_ WDFDEVICE Device,
    _In_ ULONG MessageId);

/* Callbacks handed to ucx01000 in the stack interface */

VOID
NTAPI
HubClearTtBuffer(
    _In_ UCXHUB_HUB_CONTEXT HubContext,
    _In_ UCXHUB_DEVICE_CONTEXT DeviceContext,
    _In_ UCXENDPOINT Endpoint,
    _In_ ULONG EndpointNumber,
    _In_ ULONG TtPortNumber);

VOID
NTAPI
HubNoPingResponse(
    _In_ UCXHUB_HUB_CONTEXT HubContext,
    _In_ UCXHUB_DEVICE_CONTEXT DeviceContext);

/* Descriptor validation, with the hub flavored loggers */

BOOLEAN
NTAPI
HubValidateHubConfiguration(
    _In_ HubFdo* Hub,
    _In_reads_bytes_(Length) PUSB_CONFIGURATION_DESCRIPTOR Descriptor,
    _In_ ULONG Length);

BOOLEAN
NTAPI
HubCheckUsb2HubDescriptor(
    _In_ HubFdo* Hub,
    _In_reads_bytes_(Length) PUSB_HUB_DESCRIPTOR Descriptor,
    _In_ ULONG Length);

BOOLEAN
NTAPI
HubCheckUsb3HubDescriptor(
    _In_ HubFdo* Hub,
    _In_reads_bytes_(Length) PUSB_30_HUB_DESCRIPTOR Descriptor,
    _In_ ULONG Length);

VOID
NTAPI
HubLogConfigTotalLengthMismatch(
    _In_ HubFdo* Hub);

/* WMI module */

VOID
NTAPI
HubWmiNotifyOverCurrent(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber);

/* Child PDO module */

EVT_WDF_TIMER HubEvtBandwidthRetryTimer;

/* integrator */

/** Connector map node of a port, or NULL when the port is not mapped. */
PVOID
NTAPI
HubConnectorNodeForPort(
    _In_ HubPort* Port);

/** Records a descriptor validation code against the device. */
VOID
NTAPI
HubLogDeviceValidationError(
    _In_ HubChild* Child,
    _In_ ULONG Code);

/* identity */

/** TRUE when the device lists Language among its string languages. */
BOOLEAN
NTAPI
HubIdLanguageSupported(
    _In_ HubChild* Child,
    _In_ USHORT Language);

/** Assigns device, hardware, compatible, container and instance ids to a new child PDO. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubIdAssignPdoIds(
    _In_ HubChild* Child,
    _In_ PWDFDEVICE_INIT DeviceInit);

/** Hardware ids into a PDO init and/or a multi-sz list, which is zeroed first. */
NTSTATUS
NTAPI
HubIdBuildHardwareIds(
    _In_ HubChild* Child,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _Out_opt_ PUSB_ID_STRING Ids);

/** Compatible ids into a PDO init and/or a multi-sz list, which is zeroed first. */
NTSTATUS
NTAPI
HubIdBuildCompatibleIds(
    _In_ HubChild* Child,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _Out_opt_ PUSB_ID_STRING Ids);

/** Duplicates a multi-sz id list into nonpaged pool. */
NTSTATUS
NTAPI
HubIdListCopy(
    _Out_ PUSB_ID_STRING Destination,
    _In_ const USB_ID_STRING* Source);

/** Frees a multi-sz id list; an empty list is fine. */
VOID
NTAPI
HubIdListFree(
    _Inout_ PUSB_ID_STRING Ids);

/** Answers IRP_MN_QUERY_DEVICE_TEXT on a child PDO; always completes the IRP. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubIdQueryDeviceText(
    _In_ HubChild* Child,
    _Inout_ PIRP Irp);

/** Container id the PDO got; FALSE when it got none. */
BOOLEAN
NTAPI
HubIdGetContainerId(
    _In_ HubChild* Child,
    _Out_ GUID* ContainerId);

/** Cached serial number text without its decoration, for the serial string IOCTL. */
BOOLEAN
NTAPI
HubIdGetSerialNumberText(
    _In_ HubChild* Child,
    _Outptr_result_bytebuffer_(*Length) PCWSTR* Text,
    _Out_ PULONG Length);

/** Frees the identity buffers of a device; called when the device object is destroyed. */
VOID
NTAPI
HubIdFreeDeviceIds(
    _In_ HubChild* Child);

/** Takes the connector map lock, needed around HubConnectorFindCompanion. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubConnectorLock(VOID);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubConnectorUnlock(VOID);

/** Companion port 0 or 1 sharing Port's connector, or NULL. Only valid under the map lock. */
HubPort*
NTAPI
HubConnectorFindCompanion(
    _In_ HubPort* Port,
    _In_ ULONG Index);

/* registry */

/* Why a device's UXD settings may be deleted */
enum class HubUxdEvent : ULONG
{
    Disable = 1,
    Disconnect = 2
};

/** Records hub over current in the hub hardware key, once per devnode. */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubNoteHubOverCurrent(
    _In_ HubFdo* Hub);

/** Device hardware key values read at PDO prepare hardware. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubReadDeviceHardwareValues(
    _In_ HubChild* Child);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubWriteDeviceValue(
    _In_ HubChild* Child,
    _In_ PCUNICODE_STRING Name,
    _In_ ULONG Type,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PVOID Data);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubWriteDeviceString(
    _In_ HubChild* Child,
    _In_ PCUNICODE_STRING Name,
    _In_ WDFSTRING Value);

/** Saves the MS OS 2.0 set info in usbflags when the set asks for alternate enumeration. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubCacheMsOs20SetInfo(
    _In_ HubChild* Child);

/** MsOs20Flags and EnumerationRetryCount, written at PDO start. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWriteEnumerationData(
    _In_ HubChild* Child);

/** Rereads the UXD record; UpdateRequired says whether the device id changes. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubSaveUxdState(
    _In_ HubFdo* Hub,
    _In_ HubChild* Child,
    _Out_opt_ PBOOLEAN UpdateRequired);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubPurgeUxdState(
    _In_ HubChild* Child,
    _In_ HubUxdEvent Event);

/** Replacement PnP id of a UXD reserved device, or an empty String. Always succeeds. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubQueryUxdDeviceValue(
    _In_ HubChild* Child,
    _In_ WDFSTRING String);

/** Telemetry values under the device's Ceip subkey. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubFlushSqmFlags(
    _In_ HubChild* Child);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWriteEnumerationFailureCode(
    _In_ HubChild* Child);

/** First selective suspend of the device; the registry write runs in a work item. */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubRecordSelectiveSuspend(
    _In_ HubChild* Child);

/** BootPathSurpriseRemovalCount + 1. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubBumpBootSurpriseRemovalCount(VOID);

/** This machine's dual role features for a dual role partner. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubQueryLocalDualRoleFeatures(
    _Out_ PULONG Features);

/** Frees the buffers the registry and MS OS code keep in the device. Call at destroy. */
VOID
NTAPI
HubReleaseRegistryState(
    _In_ HubChild* Child);

/* devucx */

/** Tells UCX the WDM PDO of the device; call right after the child PDO was created. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubChildSetPdo(
    _In_ HubChild* Child,
    _In_ PDEVICE_OBJECT Pdo);

/** Sends the device's current client request to UCX as is; completes it, then posts ControllerRequestDone. */
VOID
NTAPI
HubChildForwardToController(
    _In_ HubChild* Child);

/** IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME: Argument1 and its length Argument2. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubQueryControllerName(
    _In_ HubFdo* Hub,
    _Out_writes_bytes_(Length) PUSB_HUB_NAME Name,
    _In_ ULONG Length);

/** IOCTL_INTERNAL_USB_GET_BUS_INFO for the device: the 16 byte USB_BUS_NOTIFICATION at Argument1. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubQueryDeviceBusInfo(
    _In_ HubChild* Child,
    _Out_ PUSB_BUS_NOTIFICATION Notification);

/* devxfer */

/** Frees the billboard record and its descriptor copy, if any. */
VOID
NTAPI
HubFreeBillboardInfo(
    _In_ HubChild* Child);

/** Frees the buffers only the transfer module owns. Call when the device object is destroyed. */
VOID
NTAPI
HubReleaseTransferState(
    _In_ HubChild* Child);

/** Pipe of Config whose USBD pipe handle is Handle, or NULL. The caller protects Config. */
HubPipe*
NTAPI
HubFindPipeByHandle(
    _In_opt_ HubConfiguration* Config,
    _In_ USBD_PIPE_HANDLE Handle);

/* wmi */

/** Registers the GUID_USB_WMI_NODE_INFO instance of a child PDO (not for hubs or composites). */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWmiRegisterDevice(
    _In_ WDFDEVICE Pdo);

/** Registers the boot device surprise removal event on an external, non hub boot PDO. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWmiRegisterBootSurpriseRemoval(
    _In_ WDFDEVICE Pdo);

/** Fires the boot device surprise removal event, if one is registered. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWmiNotifyBootSurpriseRemoval(VOID);

/** "USB device not recognized" popup for a port. */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubWmiNotifyEnumerationFailure(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber);

/** A hub was attached beyond the allowed tier depth. */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubWmiNotifyHubNestedTooDeeply(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber);

/** The selected configuration draws more power than the port can give. */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubWmiNotifyInsufficientPower(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber);

/** The controller ran out of bandwidth for the device; fired by the out of bandwidth timer. */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubWmiNotifyInsufficientBandwidth(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber);

/** The 71 byte USB 2.0 style hub descriptor legacy tools expect, for any hub speed. */
VOID
NTAPI
HubMakeUsb2StyleDescriptor(
    _In_ HubFdo* Hub,
    _Out_ PUSB_HUB_DESCRIPTOR Descriptor);
