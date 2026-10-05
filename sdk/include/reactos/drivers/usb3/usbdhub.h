/*
 * PROJECT:     ReactOS USB Driver Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     usbd.sys exports used by the SuperSpeed hub driver
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Hub numbers run from 1 to 255; 0 means none was available */
#define USBD_HUB_NUMBER_NONE 0
#define USBD_HUB_NUMBER_COUNT 256

/* USBD_AddDeviceToGlobalList results */
#define USBD_GLOBAL_LIST_ADDED              1
#define USBD_GLOBAL_LIST_DUPLICATE_PENDING  2
#define USBD_GLOBAL_LIST_DUPLICATE          3
#define USBD_GLOBAL_LIST_NO_MEMORY          4

_IRQL_requires_(PASSIVE_LEVEL)
ULONG
NTAPI
USBD_AllocateHubNumber(VOID);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
USBD_ReleaseHubNumber(
    _In_ ULONG HubNumber);

/** Tells apart devices that claim the same VID, PID and serial number. */
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG
NTAPI
USBD_AddDeviceToGlobalList(
    _In_ PVOID DeviceContext,
    _In_opt_ PVOID HubContext,
    _In_ ULONG PortNumber,
    _In_opt_ PVOID ConnectorNode,
    _In_ USHORT IdVendor,
    _In_ USHORT IdProduct,
    _In_ PUSB_ID_STRING SerialNumber);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
USBD_MarkDeviceAsDisconnected(
    _In_ PVOID DeviceContext);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
USBD_RemoveDeviceFromGlobalList(
    _In_ PVOID DeviceContext);

#ifdef __cplusplus
}
#endif
