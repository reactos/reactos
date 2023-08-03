/*
 * PROJECT:     ReactOS USB xHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBXHCI root hub functions
 * COPYRIGHT:   Copyright 2023 Ian Marco Moffett <ian@vegaa.systems>
 */

#include "usbxhci.h"

// #define NDEBUG
#include <debug.h>

MPSTATUS
NTAPI
XHCI_RH_ChirpRootPort(IN PVOID XhciExtension,
                      IN USHORT Port)
{
    DPRINT1("XHCI_RH_ChirpRootPort: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_RH_GetRootHubData(IN PVOID XhciExtension,
                       IN PVOID RootHubData)
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
XHCI_RH_GetStatus(IN PVOID XhciExtension,
                  IN PUSHORT Status)
{
    DPRINT1("XHCI_RH_GetStatus: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_GetPortStatus(IN PVOID XhciExtension,
                      IN USHORT Port,
                      IN PUSB_PORT_STATUS_AND_CHANGE PortStatus)
{
    DPRINT1("XHCI_RH_GetPortStatus: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_GetHubStatus(IN PVOID XhciExtension,
                     IN PUSB_HUB_STATUS_AND_CHANGE HubStatus)
{
    DPRINT1("XHCI_RH_GetHubStatus: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortReset(IN PVOID XhciExtension,
                            IN USHORT Port)
{
    DPRINT1("XHCI_RH_SetFeaturePortReset: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortPower(IN PVOID XhciExtension,
                            IN USHORT Port)
{
    DPRINT1("XHCI_RH_SetFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortEnable(IN PVOID XhciExtension,
                             IN USHORT Port)
{
    DPRINT1("XHCI_RH_SetFeaturePortEnable: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortSuspend(IN PVOID XhciExtension,
                              IN USHORT Port)
{
    DPRINT1("XHCI_RH_SetFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortPower(IN PVOID XhciExtension,
                              IN USHORT Port)
{
    DPRINT1("XHCI_RH_SetFeaturePortPower: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspend(IN PVOID XhciExtension,
                                IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnable(IN PVOID XhciExtension,
                               IN USHORT Port)
{
    DPRINT1("XHCI_RH: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnableChange(IN PVOID XhciExtension,
                                     IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortEnableChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortConnectChange(IN PVOID XhciExtension,
                                      IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortConnectChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortResetChange(IN PVOID XhciExtension,
                                    IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortResetChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspendChange(IN PVOID XhciExtension,
                                      IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortSuspend: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortOvercurrentChange(IN PVOID XhciExtension,
                                          IN USHORT Port)
{
    DPRINT1("XHCI_RH_ClearFeaturePortOvercurrentChange: UNIMPLEMENTED. FIXME\n");
    return MP_STATUS_SUCCESS;
}

VOID
NTAPI
XHCI_RH_DisableIrq(IN PVOID XhciExtension)
{
    DPRINT1("XHCI_RH_DisableIrq: UNIMPLEMENTED. FIXME\n");
}

VOID
NTAPI
XHCI_RH_EnableIrq(IN PVOID XhciExtension)
{
    DPRINT1("XHCI_RH_EnableIrq: UNIMPLEMENTED. FIXME\n");
}
