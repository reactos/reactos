#ifndef USBXHCI_H__
#define USBXHCI_H__

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <wdm.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include "..\usbmport.h"
#include "hardware.h"

extern USBPORT_REGISTRATION_PACKET RegPacket;

typedef struct _XHCI_EXTENSION {
  ULONG Reserved;
  ULONG Flags;
  PULONG BaseIoAdress;
  PULONG OperationalRegs;
  UCHAR FrameLengthAdjustment;
  
} XHCI_EXTENSION, *PXHCI_EXTENSION;

typedef struct _XHCI_HC_RESOURCES {
  ULONG Reserved;
} XHCI_HC_RESOURCES, *PXHCI_HC_RESOURCES;

typedef struct _XHCI_ENDPOINT {
  ULONG Reserved;
} XHCI_ENDPOINT, *PXHCI_ENDPOINT;

typedef struct _XHCI_TRANSFER {
  ULONG Reserved;
} XHCI_TRANSFER, *PXHCI_TRANSFER;

  
//roothub functions
VOID
NTAPI
XHCI_RH_GetRootHubData(
  IN PVOID ohciExtension,
  IN PVOID rootHubData);

MPSTATUS
NTAPI
XHCI_RH_GetStatus(
  IN PVOID ohciExtension,
  IN PUSHORT Status);

MPSTATUS
NTAPI
XHCI_RH_GetPortStatus(
  IN PVOID ohciExtension,
  IN USHORT Port,
  IN PULONG PortStatus);

MPSTATUS
NTAPI
XHCI_RH_GetHubStatus(
  IN PVOID ohciExtension,
  IN PULONG HubStatus);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortReset(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortPower(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortEnable(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortSuspend(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnable(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortPower(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspend(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnableChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortConnectChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortResetChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspendChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortOvercurrentChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

VOID
NTAPI
XHCI_RH_DisableIrq(
  IN PVOID ohciExtension);

VOID
NTAPI
XHCI_RH_EnableIrq(
  IN PVOID ohciExtension);


#endif /* USBXHCI_H__ */