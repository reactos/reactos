#ifndef USBUHCI_H__
#define USBUHCI_H__

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <drivers/usbport/usbmport.h>
#include "hardware.h"

#define UHCI_MAX_HC_SCHEDULE_ERRORS        16

#define UHCI_MAX_ISO_TRANSFER_SIZE         0x10000
#define UHCI_MAX_BULK_TRANSFER_SIZE        0x1000
#define UHCI_MAX_ISO_TD_COUNT              256
#define UHCI_MAX_INTERRUPT_TD_COUNT        8

/* Host Controller Driver Transfer Descriptor (HCD TD) */
#define UHCI_HCD_TD_FLAG_ALLOCATED     0x00000001
#define UHCI_HCD_TD_FLAG_PROCESSED     0x00000002
#define UHCI_HCD_TD_FLAG_DONE          0x00000008
#define UHCI_HCD_TD_FLAG_NOT_ACCESSED  0x00000010
#define UHCI_HCD_TD_FLAG_DATA_BUFFER   0x00000020
#define UHCI_HCD_TD_FLAG_GOOD_FRAME    0x00000040
#define UHCI_HCD_TD_FLAG_CONTROLL      0x00000400
#define UHCI_HCD_TD_FLAG_STALLED_SETUP 0x00000800

typedef struct _UHCI_ENDPOINT *PUHCI_ENDPOINT;
typedef struct _UHCI_TRANSFER *PUHCI_TRANSFER;

typedef struct _UHCI_HCD_TD {
  /* Hardware */
  UHCI_TD HwTD;
  /* Software */
  USB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;
  ULONG_PTR PhysicalAddress;
  ULONG Flags;
  struct _UHCI_HCD_TD * NextHcdTD;
  _ANONYMOUS_UNION union {
    PUHCI_TRANSFER UhciTransfer;
    ULONG Frame; // for SOF_HcdTDs only
  } DUMMYUNIONNAME;
  LIST_ENTRY TdLink;
  ULONG Padded[4];
} UHCI_HCD_TD, *PUHCI_HCD_TD;

C_ASSERT(sizeof(UHCI_HCD_TD) == 0x40);

/* Host Controller Driver Queue Header (HCD QH) */
#define UHCI_HCD_QH_FLAG_ACTIVE  0x00000001
#define UHCI_HCD_QH_FLAG_REMOVE  0x00000002

typedef struct _UHCI_HCD_QH {
  /* Hardware */
  UHCI_QH HwQH;
  /* Software */
  ULONG_PTR PhysicalAddress;
  ULONG QhFlags;
  struct _UHCI_HCD_QH * NextHcdQH;
  struct _UHCI_HCD_QH * PrevHcdQH;
  PUHCI_ENDPOINT UhciEndpoint;
  ULONG Padded[9];
} UHCI_HCD_QH, *PUHCI_HCD_QH;

C_ASSERT(sizeof(UHCI_HCD_QH) == 0x40);

#define UHCI_ENDPOINT_FLAG_HALTED           1
#define UHCI_ENDPOINT_FLAG_RESERVED         2
#define UHCI_ENDPOINT_FLAG_CONTROLL_OR_ISO  4

typedef struct _UHCI_ENDPOINT {
  ULONG Flags;
  LONG EndpointLock;
  USBPORT_ENDPOINT_PROPERTIES EndpointProperties;
  PUHCI_HCD_QH QH;
  PUHCI_HCD_TD TailTD;
  PUHCI_HCD_TD HeadTD;
  PUHCI_HCD_TD FirstTD;
  ULONG MaxTDs;
  ULONG AllocatedTDs;
  ULONG AllocTdCounter;
  LIST_ENTRY ListTDs;
  BOOL DataToggle;
} UHCI_ENDPOINT, *PUHCI_ENDPOINT;

typedef struct _UHCI_TRANSFER {
  PUSBPORT_TRANSFER_PARAMETERS TransferParameters;
  PUHCI_ENDPOINT UhciEndpoint;
  USBD_STATUS USBDStatus;
  ULONG PendingTds;
} UHCI_TRANSFER, *PUHCI_TRANSFER;

#define UHCI_FRAME_LIST_POINTER_VALID      (0 << 0) 
#define UHCI_FRAME_LIST_POINTER_TERMINATE  (1 << 0) 
#define UHCI_FRAME_LIST_POINTER_TD         (0 << 1) 
#define UHCI_FRAME_LIST_POINTER_QH         (1 << 1) 

#define UHCI_FRAME_LIST_INDEX_MASK         0x3FF
#define UHCI_MAX_STATIC_SOF_TDS            8

typedef struct _UHCI_HC_RESOURCES {
  ULONG_PTR FrameList[UHCI_FRAME_LIST_MAX_ENTRIES]; // The 4-Kbyte Frame List Table is aligned on a 4-Kbyte boundary
  UHCI_HCD_QH StaticIntHead[INTERRUPT_ENDPOINTs];
  UHCI_HCD_QH StaticControlHead;
  UHCI_HCD_QH StaticBulkHead;
  UHCI_HCD_TD StaticBulkTD;
  UHCI_HCD_TD StaticTD;
  UHCI_HCD_TD StaticSofTD[UHCI_MAX_STATIC_SOF_TDS];
} UHCI_HC_RESOURCES, *PUHCI_HC_RESOURCES;

#define UHCI_EXTENSION_FLAG_SUSPENDED  0x00000002

typedef struct _UHCI_EXTENSION {
  PUHCI_HW_REGISTERS BaseRegister;
  USB_CONTROLLER_FLAVOR HcFlavor;
  PUHCI_HC_RESOURCES HcResourcesVA;
  PUHCI_HC_RESOURCES HcResourcesPA;
  PUHCI_HCD_QH IntQH[INTERRUPT_ENDPOINTs];
  PUHCI_HCD_QH ControlQH;
  PUHCI_HCD_QH BulkQH;
  PUHCI_HCD_QH BulkTailQH;
  PUHCI_HCD_TD StaticTD;
  PUHCI_HCD_TD SOF_HcdTDs; // pointer to array StaticSofTD[UHCI_MAX_STATIC_SOF_TDS]
  ULONG FrameNumber;
  ULONG FrameHighPart;
  ULONG Flags;
  LONG LockFrameList;
  ULONG ResetPortMask;
  ULONG ResetChangePortMask;
  ULONG SuspendChangePortMask;
  ULONG HcScheduleError;
  LONG ExtensionLock;

  UHCI_USB_STATUS StatusMask;
  UHCI_USB_STATUS HcStatus;
  UCHAR SOF_Modify;
  UCHAR Padded2[3];
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
  IN PUSB_PORT_STATUS_AND_CHANGE PortStatus);

MPSTATUS
NTAPI
UhciRHGetHubStatus(
  IN PVOID uhciExtension,
  IN PUSB_HUB_STATUS_AND_CHANGE HubStatus);

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
VOID
NTAPI
UhciDisableInterrupts(
  IN PVOID uhciExtension);

ULONG
NTAPI
UhciGet32BitFrameNumber(
  IN PVOID uhciExtension);

BOOLEAN
NTAPI
UhciHardwarePresent(
  IN PUHCI_EXTENSION UhciExtension);

#endif /* USBUHCI_H__ */
