#ifndef USBOHCI_H__
#define USBOHCI_H__

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

#define OHCI_HCD_ED_FLAG_RESET_ON_HALT 0x00000008

#define OHCI_HCD_TD_FLAG_ALLOCATED     0x00000001
#define OHCI_HCD_TD_FLAG_PROCESSED     0x00000002
#define OHCI_HCD_TD_FLAG_CONTROLL      0x00000004
#define OHCI_HCD_TD_FLAG_DONE          0x00000008

typedef struct _OHCI_TRANSFER *POHCI_TRANSFER;

typedef union _OHCI_HW_TRANSFER_DESCRIPTOR {
  struct {
    OHCI_TRANSFER_DESCRIPTOR gTD; // must be aligned to a 16-byte boundary
    USB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;
    ULONG Padded[2];
  };
  struct {
    OHCI_ISO_TRANSFER_DESCRIPTOR iTD; // must be aligned to a 32-byte boundary
  };
} OHCI_HW_TRANSFER_DESCRIPTOR, *POHCI_HW_TRANSFER_DESCRIPTOR;

C_ASSERT(sizeof(OHCI_HW_TRANSFER_DESCRIPTOR) == 32);

typedef struct _OHCI_HCD_TD {
  // Hardware part
  OHCI_HW_TRANSFER_DESCRIPTOR HwTD; // must be aligned to a 32-byte boundary
  // Software part
  ULONG_PTR PhysicalAddress;
  ULONG Flags;
  POHCI_TRANSFER OhciTransfer;
  struct _OHCI_HCD_TD * NextHcdTD;
  ULONG TransferLen;
  LIST_ENTRY DoneLink;
  ULONG Pad[1];
} OHCI_HCD_TD, *POHCI_HCD_TD;

C_ASSERT(sizeof(OHCI_HCD_TD) == 0x40);

typedef struct _OHCI_HCD_ED {
  // Hardware part
  OHCI_ENDPOINT_DESCRIPTOR HwED; // must be aligned to a 16-byte boundary
  // Software part
  ULONG_PTR PhysicalAddress;
  ULONG Flags;
  LIST_ENTRY HcdEDLink;
  ULONG Pad[8];
} OHCI_HCD_ED, *POHCI_HCD_ED;

C_ASSERT(sizeof(OHCI_HCD_ED) == 0x40);

typedef struct _OHCI_STATIC_ED {
  // Software only part
  POHCI_ENDPOINT_DESCRIPTOR HwED;
  ULONG_PTR PhysicalAddress;
  UCHAR HeadIndex;
  UCHAR Reserved[3];
  LIST_ENTRY Link;
  ULONG Type;
  PULONG pNextED;
  ULONG HccaIndex;
} OHCI_STATIC_ED, *POHCI_STATIC_ED;

/* roothub.c */

VOID
NTAPI
OHCI_RH_GetRootHubData(
  IN PVOID ohciExtension,
  IN PVOID rootHubData);

MPSTATUS
NTAPI
OHCI_RH_GetStatus(
  IN PVOID ohciExtension,
  IN PUSHORT Status);

MPSTATUS
NTAPI
OHCI_RH_GetPortStatus(
  IN PVOID ohciExtension,
  IN USHORT Port,
  IN PULONG PortStatus);

MPSTATUS
NTAPI
OHCI_RH_GetHubStatus(
  IN PVOID ohciExtension,
  IN PULONG HubStatus);

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortReset(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortPower(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortEnable(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_SetFeaturePortSuspend(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortEnable(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortPower(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortSuspend(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortEnableChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortConnectChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortResetChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortSuspendChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
OHCI_RH_ClearFeaturePortOvercurrentChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

VOID
NTAPI
OHCI_RH_DisableIrq(
  IN PVOID ohciExtension);

VOID
NTAPI
OHCI_RH_EnableIrq(
  IN PVOID ohciExtension);

/* usbohci.c */

NTSTATUS
NTAPI
USBPORT_RegisterUSBPortDriver(
  IN PDRIVER_OBJECT DriverObject,
  IN ULONG Version,
  IN PVOID RegistrationPacket);

#endif /* USBOHCI_H__ */
