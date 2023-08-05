/*
 * PROJECT:     ReactOS USB xHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBXHCI root hub functions
 * COPYRIGHT:   Copyright 2023 Ian Marco Moffett <ian@vegaa.systems>
 */

#include "usbxhci.h"

#define NDEBUG
#include <debug.h>

MPSTATUS
NTAPI
XHCI_RH_ChirpRootPort(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_ChirpRootPort: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_RH_GetRootHubData(
    _In_ PVOID XhciExtension,
    _In_ PVOID RootHubData)
{
    PUSBPORT_ROOT_HUB_DATA RH_Data;
    PXHCI_EXTENSION XhciExt;

    RH_Data = (PUSBPORT_ROOT_HUB_DATA)RootHubData;
    RtlZeroMemory(RH_Data, sizeof(*RH_Data));

    XhciExt = (PXHCI_EXTENSION)XhciExtension;
    RH_Data->NumberOfPorts = XhciExt->NumberOfPorts;
}

MPSTATUS
NTAPI
XHCI_RH_GetStatus(
    _In_ PVOID XhciExtension,
    _Out_ PUSHORT Status)
{
    DPRINT("XHCI_RH_GetStatus: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_GetPortStatus(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port,
    _In_ PUSB_PORT_STATUS_AND_CHANGE PortStatus)
{
    DPRINT("XHCI_RH_GetPortStatus: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_GetHubStatus(
    _In_ PVOID XhciExtension,
    _In_ PUSB_HUB_STATUS_AND_CHANGE HubStatus)
{
    DPRINT("XHCI_RH_GetHubStatus: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortReset(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_SetFeaturePortReset: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortPower(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_SetFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortEnable(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_SetFeaturePortEnable: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortSuspend(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_SetFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortPower(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_SetFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspend(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_ClearFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnable(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnableChange(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_ClearFeaturePortEnableChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortConnectChange(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_ClearFeaturePortConnectChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortResetChange(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_ClearFeaturePortResetChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspendChange(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_ClearFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortOvercurrentChange(
    _In_ PVOID XhciExtension,
    _In_ USHORT Port)
{
    DPRINT("XHCI_RH_ClearFeaturePortOvercurrentChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_RH_DisableIrq(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_RH_DisableIrq: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_RH_EnableIrq(_In_ PVOID XhciExtension)
{
    DPRINT("XHCI_RH_EnableIrq: UNIMPLEMENTED. FIXME\n");
}
