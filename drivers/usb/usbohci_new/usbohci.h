/*
 * PROJECT:     ReactOS USB OHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBOHCI declarations
 * COPYRIGHT:   Copyright 2017-2018 Vadim Galyant <vgal@rambler.ru>
 */

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

#define OHCI_HCD_ED_FLAG_CONTROL       0x00000001
#define OHCI_HCD_ED_FLAG_RESET_ON_HALT 0x00000008
#define OHCI_HCD_ED_FLAG_NOT_ACCESSED  0x00000010

#define OHCI_HCD_TD_FLAG_ALLOCATED       0x00000001
#define OHCI_HCD_TD_FLAG_PROCESSED       0x00000002
#define OHCI_HCD_TD_FLAG_CONTROL_STATUS  0x00000004
#define OHCI_HCD_TD_FLAG_DONE            0x00000008
#define OHCI_HCD_TD_FLAG_NOT_ACCESSED    0x00000010

#define OHCI_MAX_ISO_TRANSFER_SIZE        0x10000
#define OHCI_MAX_CONTROL_TRANSFER_SIZE    0x10000
#define OHCI_MAX_BULK_TRANSFER_SIZE       0x40000
#define OHCI_MAX_INTERRUPT_TRANSFER_SIZE  0x1000

#define OHCI_MAX_ISO_TD_COUNT        64
#define OHCI_MAX_CONTROL_TD_COUNT    38
#define OHCI_MAX_BULK_TD_COUNT       68
#define OHCI_MAX_INTERRUPT_TD_COUNT  4

#define OHCI_NON_DATA_CONTROL_TDS  2

#define OHCI_TRANSFER_FLAGS_SHORT_TRANSFER_OK  1

typedef struct _OHCI_TRANSFER *POHCI_TRANSFER;

typedef union DECLSPEC_ALIGN(32) _OHCI_HW_TRANSFER_DESCRIPTOR {
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
C_ASSERT(__alignof(OHCI_HW_TRANSFER_DESCRIPTOR) == 32);

typedef struct _OHCI_HCD_TD {
  /* Hardware part */
  OHCI_HW_TRANSFER_DESCRIPTOR HwTD; // must be aligned to a 32-byte boundary
  /* Software part */
  ULONG PhysicalAddress;
  ULONG Flags;
  POHCI_TRANSFER OhciTransfer;
  struct _OHCI_HCD_TD *NextTDVa;
  ULONG TransferLen;
  LIST_ENTRY DoneLink;
  ULONG Pad[1];
} OHCI_HCD_TD, *POHCI_HCD_TD;

#ifdef _WIN64
C_ASSERT(sizeof(OHCI_HCD_TD) == 96);
#else
C_ASSERT(sizeof(OHCI_HCD_TD) == 64);
#endif

typedef struct _OHCI_HCD_ED {
  /* Hardware part */
  OHCI_ENDPOINT_DESCRIPTOR HwED; // must be aligned to a 16-byte boundary
  /* Software part */
  ULONG PhysicalAddress;
  ULONG Flags;
  LIST_ENTRY HcdEDLink;
#ifdef _WIN64
  ULONG Pad[6];
#else
  ULONG Pad[8];
#endif
} OHCI_HCD_ED, *POHCI_HCD_ED;

C_ASSERT(sizeof(OHCI_HCD_ED) == 64);

#define OHCI_STATIC_ED_TYPE_INTERRUPT  0
#define OHCI_STATIC_ED_TYPE_CONTROL    1
#define OHCI_STATIC_ED_TYPE_BULK       2

typedef struct _OHCI_STATIC_ED {
  /* Software only */
  POHCI_ENDPOINT_DESCRIPTOR HwED;
  ULONG PhysicalAddress;
  UCHAR HeadIndex;
  UCHAR Reserved[3];
  LIST_ENTRY Link;
  ULONG Type;
  PULONG pNextED;
  ULONG HccaIndex;
} OHCI_STATIC_ED, *POHCI_STATIC_ED;

typedef struct _OHCI_HC_RESOURCES {
  OHCI_HCCA HcHCCA; // (256 byte align)
  OHCI_ENDPOINT_DESCRIPTOR InterrruptHeadED[63]; // (16 byte align)
  OHCI_ENDPOINT_DESCRIPTOR ControlHeadED; // (16 byte align)
  OHCI_ENDPOINT_DESCRIPTOR BulkHeadED; // (16 byte align)
} OHCI_HC_RESOURCES, *POHCI_HC_RESOURCES;

/* OHCI Endpoint follows USBPORT Endpoint */
typedef struct _OHCI_ENDPOINT {
  ULONG Reserved;
  USBPORT_ENDPOINT_PROPERTIES EndpointProperties;
  POHCI_STATIC_ED HeadED;
  POHCI_HCD_TD FirstTD;
  POHCI_HCD_ED HcdED;
  ULONG MaxTransferDescriptors;
  POHCI_HCD_TD HcdHeadP;
  POHCI_HCD_TD HcdTailP;
  LIST_ENTRY TDList;
} OHCI_ENDPOINT, *POHCI_ENDPOINT;

/* OHCI Transfer follows USBPORT Transfer */
typedef struct _OHCI_TRANSFER {
  ULONG Reserved;
  ULONG TransferLen;
  PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
  ULONG PendingTDs;
  ULONG Flags;
  USBD_STATUS USBDStatus;
  POHCI_ENDPOINT OhciEndpoint;
  POHCI_HCD_TD NextTD;
  POHCI_HCD_TD ControlStatusTD;
} OHCI_TRANSFER, *POHCI_TRANSFER;

/* OHCI Extension follows USBPORT Extension */
typedef struct _OHCI_EXTENSION {
  ULONG Reserved;
  POHCI_OPERATIONAL_REGISTERS OperationalRegs;
  OHCI_REG_FRAME_INTERVAL FrameInterval;
  ULONG FrameHighPart;
  ULONG HcdFmNumber;
  POHCI_HC_RESOURCES HcResourcesVA;
  ULONG HcResourcesPA;
  OHCI_STATIC_ED IntStaticED[63];
  OHCI_STATIC_ED ControlStaticED;
  OHCI_STATIC_ED BulkStaticED;
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
  IN PUSB_PORT_STATUS_AND_CHANGE PortStatus);

MPSTATUS
NTAPI
OHCI_RH_GetHubStatus(
  IN PVOID ohciExtension,
  IN PUSB_HUB_STATUS_AND_CHANGE HubStatus);

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

OHCI_REG_RH_DESCRIPTORA
NTAPI
OHCI_ReadRhDescriptorA(
  IN POHCI_EXTENSION OhciExtension);

#endif /* USBOHCI_H__ */
