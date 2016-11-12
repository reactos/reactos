#ifndef USBEHCI_H__
#define USBEHCI_H__

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <wdm.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <drivers/usbport/usbmport.h>

#include "hardware.h"

extern USBPORT_REGISTRATION_PACKET RegPacket;

typedef struct _EHCI_HCD_TD {
  //Hardware
  EHCI_QUEUE_TD HwTD;
  //Software
  struct _EHCI_HCD_TD * PhysicalAddress;
  ULONG TdFlags;
  ULONG Pad[49];
} EHCI_HCD_TD, *PEHCI_HCD_TD;

C_ASSERT(sizeof(EHCI_HCD_TD) == 0x100);

typedef struct _EHCI_HCD_QH {
  //Hardware
  EHCI_QUEUE_HEAD HwQH;
  //Software
  struct _EHCI_HCD_QH * PhysicalAddress;
  ULONG QhFlags;
  ULONG Pad[49];
} EHCI_HCD_QH, *PEHCI_HCD_QH;

C_ASSERT(sizeof(EHCI_HCD_QH) == 0x100);

typedef struct _EHCI_ENDPOINT {
  ULONG Reserved;
} EHCI_ENDPOINT, *PEHCI_ENDPOINT;

typedef struct _EHCI_TRANSFER {
  ULONG Reserved;
} EHCI_TRANSFER, *PEHCI_TRANSFER;

typedef struct _EHCI_STATIC_QH {
  //Hardware
  EHCI_QUEUE_HEAD HwQH;
  //Software
  struct _EHCI_STATIC_QH * PhysicalAddress;
  ULONG QhFlags;
  ULONG Pad[25];
} EHCI_STATIC_QH, *PEHCI_STATIC_QH;

C_ASSERT(sizeof(EHCI_STATIC_QH) == 0xA0);

typedef struct _EHCI_HC_RESOURCES {
  PEHCI_STATIC_QH PeriodicFrameList[1024]; // 4K-page aligned array
  EHCI_STATIC_QH AsyncHead;
  EHCI_STATIC_QH PeriodicHead[64];
  UCHAR Padded[0x160];
  EHCI_HCD_QH DummyQH[1024];
} EHCI_HC_RESOURCES, *PEHCI_HC_RESOURCES;

typedef struct _EHCI_EXTENSION {
  ULONG Reserved;
  ULONG Flags;
  PULONG BaseIoAdress;
  PULONG OperationalRegs;
  UCHAR FrameLengthAdjustment;
  BOOLEAN IsStarted;
  USHORT Reserved1;
  ULONG PortRoutingControl;
  USHORT NumberOfPorts; // HCSPARAMS => N_PORTS 
  USHORT PortPowerControl; // HCSPARAMS => Port Power Control (PPC)

} EHCI_EXTENSION, *PEHCI_EXTENSION;

/* roothub.c */

VOID
NTAPI
EHCI_RH_GetRootHubData(
  IN PVOID ohciExtension,
  IN PVOID rootHubData);

MPSTATUS
NTAPI
EHCI_RH_GetStatus(
  IN PVOID ohciExtension,
  IN PUSHORT Status);

MPSTATUS
NTAPI
EHCI_RH_GetPortStatus(
  IN PVOID ohciExtension,
  IN USHORT Port,
  IN PULONG PortStatus);

MPSTATUS
NTAPI
EHCI_RH_GetHubStatus(
  IN PVOID ohciExtension,
  IN PULONG HubStatus);

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortReset(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortPower(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortEnable(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_SetFeaturePortSuspend(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortEnable(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortPower(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortSuspend(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortEnableChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortConnectChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortResetChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortSuspendChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
EHCI_RH_ClearFeaturePortOvercurrentChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

VOID
NTAPI
EHCI_RH_DisableIrq(
  IN PVOID ohciExtension);

VOID
NTAPI
EHCI_RH_EnableIrq(
  IN PVOID ohciExtension);

/* usbehci.c */

ULONG
NTAPI
USBPORT_GetHciMn(VOID);

NTSTATUS
NTAPI
USBPORT_RegisterUSBPortDriver(
  IN PDRIVER_OBJECT DriverObject,
  IN ULONG Version,
  IN PUSBPORT_REGISTRATION_PACKET RegistrationPacket);

#endif /* USBEHCI_H__ */
