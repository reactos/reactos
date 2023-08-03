/*
 * PROJECT:     ReactOS USB XHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBXHCI declarations
 * COPYRIGHT:   Copyright 2023 Ian Marco Moffett <ian@vegaa.systems>
 */

#pragma once

/* Max command ring entries */
#define XHCI_COMMAND_RING_MAX 128

#include <ntddk.h>
#include <windef.h>
#include <hubbusif.h>
#include <drivers/usbport/usbmport.h>
#include "hardware.h"

extern USBPORT_REGISTRATION_PACKET RegPacket;

typedef struct _XHCI_HC_RESOURCES {
    ULONG Dcbaa[256];
    XHCI_COMMAND_TRB CommandRing[XHCI_COMMAND_RING_MAX];
} XHCI_HC_RESOURCES, *PXHCI_HC_RESOURCES;

typedef struct _XHCI_EXTENSION {
    ULONG Rsvd;
    ULONG Flags;
    PXHCI_HC_CAPABILITY_REGISTERS CapRegs;
    PXHCI_HC_OPER_REGS OperRegs;
    UCHAR FrameLengthAdjustment;
    BOOLEAN IsStarted;
    USHORT HcSystemErrors;
    ULONG PortRoutingControl;
    USHORT NumberOfPorts;
    USHORT PortPowerControl;
} XHCI_EXTENSION, *PXHCI_EXTENSION;

/* roothub.c */
MPSTATUS
NTAPI
XHCI_RH_ChirpRootPort(
  IN PVOID XhciExtension,
  IN USHORT Port);

VOID
NTAPI
XHCI_RH_GetRootHubData(
  IN PVOID XhciExtension,
  IN PVOID rootHubData);

MPSTATUS
NTAPI
XHCI_RH_GetStatus(
  IN PVOID XhciExtension,
  IN PUSHORT Status);

MPSTATUS
NTAPI
XHCI_RH_GetPortStatus(
  IN PVOID XhciExtension,
  IN USHORT Port,
  IN PUSB_PORT_STATUS_AND_CHANGE PortStatus);

MPSTATUS
NTAPI
XHCI_RH_GetHubStatus(
  IN PVOID XhciExtension,
  IN PUSB_HUB_STATUS_AND_CHANGE HubStatus);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortReset(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortPower(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortEnable(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortSuspend(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnable(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortPower(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspend(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnableChange(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortConnectChange(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortResetChange(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspendChange(
  IN PVOID XhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortOvercurrentChange(
  IN PVOID XhciExtension,
  IN USHORT Port);

VOID
NTAPI
XHCI_RH_DisableIrq(
  IN PVOID XhciExtension);

VOID
NTAPI
XHCI_RH_EnableIrq(
    IN PVOID XhciExtension);
