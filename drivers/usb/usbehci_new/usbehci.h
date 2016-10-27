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

extern USBPORT_REGISTRATION_PACKET RegPacket;

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
