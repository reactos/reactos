#ifndef USBUHCI_H__
#define USBUHCI_H__

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

/* roothub.c */

VOID
NTAPI
UhciRHGetRootHubData(
  IN PVOID uhciExtension,
  IN PVOID rootHubData);

MPSTATUS
NTAPI
UhciRHGetStatus(
  IN PVOID uhciExtension,
  IN PUSHORT Status);

MPSTATUS
NTAPI
UhciRHGetPortStatus(
  IN PVOID uhciExtension,
  IN USHORT Port,
  IN PULONG PortStatus);

MPSTATUS
NTAPI
UhciRHGetHubStatus(
  IN PVOID uhciExtension,
  IN PULONG HubStatus);

MPSTATUS
NTAPI
UhciRHSetFeaturePortReset(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHSetFeaturePortPower(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHSetFeaturePortEnable(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHSetFeaturePortSuspend(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHClearFeaturePortEnable(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHClearFeaturePortPower(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHClearFeaturePortSuspend(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHClearFeaturePortEnableChange(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHClearFeaturePortConnectChange(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHClearFeaturePortResetChange(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHClearFeaturePortSuspendChange(
  IN PVOID uhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
UhciRHClearFeaturePortOvercurrentChange(
  IN PVOID uhciExtension,
  IN USHORT Port);

VOID
NTAPI
UhciRHDisableIrq(
  IN PVOID uhciExtension);

VOID
NTAPI
UhciRHEnableIrq(
  IN PVOID uhciExtension);

/* usbuhci.c */

NTSTATUS
NTAPI
USBPORT_RegisterUSBPortDriver(
  IN PDRIVER_OBJECT DriverObject,
  IN ULONG Version,
  IN PVOID RegistrationPacket);

#endif /* USBUHCI_H__ */
