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

/* Host Controller Driver Transfer Descriptor (HCD TD) */

typedef struct _UHCI_HCD_TD {
  /* Hardware */
  UHCI_TD HwTD;
  /* Software */
  struct _UHCI_HCD_TD * PhysicalAddress;
  ULONG Padded[11];
} UHCI_HCD_TD, *PUHCI_HCD_TD;

C_ASSERT(sizeof(UHCI_HCD_TD) == 0x40);

/* Host Controller Driver Queue Header (HCD QH) */

typedef struct _UHCI_HCD_QH {
  /* Hardware */
  UHCI_QH HwQH;
  /* Software */
  struct _UHCI_HCD_QH * PhysicalAddress;
  ULONG Padded[13];
} UHCI_HCD_QH, *PUHCI_HCD_QH;

C_ASSERT(sizeof(UHCI_HCD_QH) == 0x40);

typedef struct _UHCI_ENDPOINT {
  ULONG Reserved;
} UHCI_ENDPOINT, *PUHCI_ENDPOINT;

typedef struct _UHCI_TRANSFER {
  ULONG Reserved;
} UHCI_TRANSFER, *PUHCI_TRANSFER;

typedef struct _UHCI_HC_RESOURCES {
  UCHAR Padded[0x2000];
} UHCI_HC_RESOURCES, *PUHCI_HC_RESOURCES;

typedef struct _UHCI_EXTENSION {
  ULONG Reserved;
} UHCI_EXTENSION, *PUHCI_EXTENSION;

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

#endif /* USBUHCI_H__ */
