#ifndef USBOHCI_H__
#define USBOHCI_H__

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
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

typedef struct _OHCI_HC_RESOURCES {
  OHCI_HCCA HcHCCA; // +0x000 (256 byte align)
  OHCI_ENDPOINT_DESCRIPTOR InterrruptHeadED[63]; // +0x100 (16 byte align)
  OHCI_ENDPOINT_DESCRIPTOR ControlHeadED; // +0x4F0 (16 byte align)
  OHCI_ENDPOINT_DESCRIPTOR BulkHeadED; // +0x500 (16 byte align)
} OHCI_HC_RESOURCES, *POHCI_HC_RESOURCES;

typedef struct _OHCI_ENDPOINT {
  ULONG Reserved;
  USBPORT_ENDPOINT_PROPERTIES EndpointProperties;
  POHCI_STATIC_ED HeadED;
  POHCI_HCD_TD FirstTD;
  POHCI_HCD_ED HcdED;
  ULONG MaxTransferDescriptors; // TdCount
  POHCI_HCD_TD HcdHeadP;
  POHCI_HCD_TD HcdTailP;
  LIST_ENTRY TDList;
} OHCI_ENDPOINT, *POHCI_ENDPOINT;

typedef struct _OHCI_TRANSFER {
  ULONG Reserved;
  ULONG TransferLen;
  PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
  ULONG PendingTDs;
  ULONG Flags;
  USBD_STATUS USBDStatus;
  POHCI_ENDPOINT OhciEndpoint;
  POHCI_HCD_TD NextTD;
} OHCI_TRANSFER, *POHCI_TRANSFER;

typedef struct _OHCI_EXTENSION {
  ULONG Reserved;
  POHCI_OPERATIONAL_REGISTERS OperationalRegs;
  OHCI_REG_FRAME_INTERVAL FrameInterval;
  ULONG FrameHighPart;
  ULONG HcdFmNumber;
  ULONG_PTR HcResourcesVA;
  ULONG_PTR HcResourcesPA;
  PVOID ScheduleStartVA;
  PVOID ScheduleStartPA;
  OHCI_STATIC_ED IntStaticED[63];
  OHCI_STATIC_ED ControlStaticED; // [64-1] ED_CONTROL
  OHCI_STATIC_ED BulkStaticED; // [65-1] ED_BULK
} OHCI_EXTENSION, *POHCI_EXTENSION;

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
  IN PUSBHUB_PORT_STATUS PortStatus);

MPSTATUS
NTAPI
OHCI_RH_GetHubStatus(
  IN PVOID ohciExtension,
  IN PUSB_HUB_STATUS HubStatus);

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

#endif /* USBOHCI_H__ */
