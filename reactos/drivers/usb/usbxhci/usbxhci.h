#ifndef USBXHCI_H__
#define USBXHCI_H__

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include "..\usbmport.h"
#include "hardware.h"

extern USBPORT_REGISTRATION_PACKET RegPacket;

#define XHCI_FLAGS_CONTROLLER_SUSPEND 0x01
//Data structures
typedef struct  _XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY {
   PHYSICAL_ADDRESS ContextBaseAddr [256];
} XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY, *PXHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY;
//----------------------------------------LINK TRB--------------------------------------------------------------------
typedef union _XHCI_LINK_TRB{
    struct {
        ULONG RsvdZ1                     : 4;
        ULONG RingSegmentPointerLo       : 28;
    };
    struct {
        ULONG RingSegmentPointerHi       : 32;
    };
    struct {
        ULONG RsvdZ2                     : 22;
        ULONG InterrupterTarget          : 10;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG ToggleCycle               : 1;
        ULONG RsvdZ3                    : 2;
        ULONG ChainBit                  : 1;
        ULONG InterruptOnCompletion     : 1;
        ULONG RsvdZ4                    : 4;
        ULONG TRBType                   : 6;
        ULONG RsvdZ5                    : 16;
    };
    ULONG AsULONG;
} XHCI_LINK_TRB;
//----------------------------------------generic trb----------------------------------------------------------------
typedef struct _XHCI_GENERIC_TRB {
    ULONG Word0;
    ULONG Word1;
    ULONG Word2;
    ULONG Word3;
}XHCI_GENERIC_TRB, *PXHCI_GENERIC_TRB;
//----------------------------------------Command TRBs----------------------------------------------------------------
typedef struct _XHCI_COMMAND_NO_OP_TRB {
        ULONG RsvdZ1;
        ULONG RsvdZ2;
        ULONG RsvdZ3;
        struct{
            ULONG CycleBit                  : 1;
            ULONG RsvdZ4                    : 4;
            ULONG TRBType                   : 6;
            ULONG RsvdZ5                    : 14;
        };
} XHCI_COMMAND_NO_OP_TRB;

typedef union _XHCI_COMMAND_TRB {
    XHCI_COMMAND_NO_OP_TRB NoOperation;
    XHCI_LINK_TRB Link[4];
    XHCI_GENERIC_TRB GenericTRB;
}XHCI_COMMAND_TRB, *PXHCI_COMMAND_TRB;

typedef struct _XHCI_COMMAND_RING {
    XHCI_COMMAND_TRB Segment[4];
    PXHCI_COMMAND_TRB CREnquePointer;
    PXHCI_COMMAND_TRB CRDequePointer;
} XHCI_COMMAND_RING;
//----------------------------------------CONTROL TRANSFER DATA STRUCTUERS--------------------------------------------

typedef union _XHCI_CONTROL_SETUP_TRB {
    struct {
        ULONG bmRequestType             : 8;
        ULONG bRequest                  : 8;
        ULONG wValue                    : 16;
    };
    struct {
        ULONG wIndex                    : 16;
        ULONG wLength                   : 16;
    };
    struct {
        ULONG TRBTransferLength         : 17;
        ULONG RsvdZ                     : 5;
        ULONG InterrupterTarget         : 10;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG RsvdZ1                    : 4;
        ULONG InterruptOnCompletion     : 1;
        ULONG ImmediateData             : 1;
        ULONG RsvdZ2                    : 3;
        ULONG TRBType                   : 6;
        ULONG TransferType              : 2;
        ULONG RsvdZ3                    : 14;
    };
    ULONG AsULONG;
} XHCI_CONTROL_SETUP_TRB;

typedef union _XHCI_CONTROL_DATA_TRB {
    struct {
        ULONG DataBufferPointerLo       : 32;
    };
    struct {
        ULONG DataBufferPointerHi       : 32;
    };
    struct {
        ULONG TRBTransferLength         : 17;
        ULONG TDSize                    : 5;
        ULONG InterrupterTarget         : 10;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG EvaluateNextTRB           : 1;
        ULONG InterruptOnShortPacket    : 1;
        ULONG NoSnoop                   : 1;
        ULONG ChainBit                  : 1;
        ULONG InterruptOnCompletion     : 1;
        ULONG ImmediateData             : 1;
        ULONG RsvdZ1                    : 2;
        ULONG TRBType                   : 6;
        ULONG Direction                 : 1;
        ULONG RsvdZ2                    : 15;
    };
    ULONG AsULONG;
} XHCI_CONTROL_DATA_TRB;

typedef union _XHCI_CONTROL_STATUS_TRB {
    struct {
        ULONG RsvdZ1                    : 32;
    };
    struct {
        ULONG RsvdZ2                    : 32;
    };
    struct {
        ULONG RsvdZ                     : 22;
        ULONG InterrupterTarget         : 10;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG EvaluateNextTRB           : 1;
        ULONG ChainBit                  : 2;
        ULONG InterruptOnCompletion     : 1;
        ULONG RsvdZ3                    : 4;
        ULONG TRBType                   : 6;
        ULONG Direction                 : 1;
        ULONG RsvdZ4                    : 15;
    };
    ULONG AsULONG;
} XHCI_CONTROL_STATUS_TRB;

typedef union _XHCI_CONTROL_TRB {
    XHCI_CONTROL_SETUP_TRB  SetupTRB[4];
    XHCI_CONTROL_DATA_TRB   DataTRB[4];
    XHCI_CONTROL_STATUS_TRB StatusTRB[4];
} XHCI_CONTROL_TRB, *PXHCI_CONTROL_TRB;  

//----------------event strucs-------------------
typedef struct _XHCI_EVENT_TRB {
    ULONG Word0;
    ULONG Word1;
    ULONG Word2;
    ULONG Word3;
}XHCI_EVENT_TRB, *PXHCI_EVENT_TRB;

typedef struct _XHCI_EVENT_RING_SEGMENT_TABLE{
    ULONGLONG RingSegmentBaseAddr;
    struct {
        ULONGLONG RingSegmentSize : 16;
        ULONGLONG RsvdZ           :  48;
    };
    
    
} XHCI_EVENT_RING_SEGMENT_TABLE;
//------------------------------------main structs-----------------------

typedef union _XHCI_TRB {
    XHCI_COMMAND_TRB    CommandTRB;
    XHCI_LINK_TRB       LinkTRB;
    XHCI_CONTROL_TRB    ControlTRB;
    XHCI_EVENT_TRB      EventTRB;
} XHCI_TRB, *PXHCI_TRB;

typedef struct _XHCI_RING {
    XHCI_TRB XhciTrb[16];
    //PXHCI_TRB dequeue_pointer;
}XHCI_RING , *PXHCI_RING;

typedef struct _XHCI_EXTENSION {
  ULONG Reserved;
  ULONG Flags;
  PULONG BaseIoAdress;
  PULONG OperationalRegs;
  PULONG RunTimeRegisterBase;
  PULONG DoorBellRegisterBase;
  UCHAR FrameLengthAdjustment;
  BOOLEAN IsStarted;
  USHORT HcSystemErrors;
  ULONG PortRoutingControl;
  USHORT NumberOfPorts; // HCSPARAMS1 => N_PORTS 
  USHORT PortPowerControl; // HCSPARAMS => Port Power Control (PPC)
  
} XHCI_EXTENSION, *PXHCI_EXTENSION;

typedef struct _XHCI_HC_RESOURCES {
  XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY DCBAA;
  //XHCI_COMMAND_RING CommandRing;
  XHCI_RING         EventRing;
  XHCI_RING         CommandRing;
  XHCI_EVENT_RING_SEGMENT_TABLE EventRingSegTable;
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
  IN PVOID xhciExtension,
  IN PVOID rootHubData);

MPSTATUS
NTAPI
XHCI_RH_GetStatus(
  IN PVOID xhciExtension,
  IN PUSHORT Status);

MPSTATUS
NTAPI
XHCI_RH_GetPortStatus(
  IN PVOID xhciExtension,
  IN USHORT Port,
  IN PULONG PortStatus);

MPSTATUS
NTAPI
XHCI_RH_GetHubStatus(
  IN PVOID xhciExtension,
  IN PULONG HubStatus);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortReset(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortPower(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortEnable(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortSuspend(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnable(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortPower(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspend(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnableChange(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortConnectChange(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortResetChange(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspendChange(
  IN PVOID xhciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortOvercurrentChange(
  IN PVOID xhciExtension,
  IN USHORT Port);

VOID
NTAPI
XHCI_RH_DisableIrq(
  IN PVOID xhciExtension);

VOID
NTAPI
XHCI_RH_EnableIrq(
  IN PVOID xhciExtension);


#endif /* USBXHCI_H__ */