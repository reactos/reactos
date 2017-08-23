#ifndef USBXHCI_H__
#define USBXHCI_H__

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <usbport/usbmport.h>
#include "hardware.h"

extern USBPORT_REGISTRATION_PACKET RegPacket;

#define XHCI_FLAGS_CONTROLLER_SUSPEND 0x01

// TRB Types
// transfer TRBs ids
#define NORMAL          1
#define SETUP_STAGE     2
#define DATA_STAGE      3
#define STATUS_STAGE    4
#define ISOCH           5
#define LINK            6  // BOTH TRNASFER AND COMMAND TRB TYPE
#define EVENT_DATA      7
#define NO_OP           8

// COMMAND TRB IDs
#define ENABLE_SLOT_COMMAND             9
#define DISABLE_SLOT_COMMAND            10
#define ADDRESS_DEVICE_COMMAND          11
#define CONFIGURE_ENDPOINT_COMMAND      12
#define EVALUATE_CONTEXT_COMMAND        13
#define RESET_ENDPOINT_COMMAND          14
#define STOP_ENDPOINT_COMMAND           15
#define SET_TR_DEQUEUE_COMMAND          16
#define RESET_DEVICE_COMMAND            17
#define FORCE_EVENT_COMMMAND            18
#define NEGOTIATE_BANDWIDTH_COMMAND     19
#define SET_LATENCY_TOLERANCE_COMMAND   20
#define GET_PORT_BANDWIDTH_COMMAND      21
#define FORCE_HEADER_COMMAND            22
#define NO_OP_COMMAND                   23

// EVENT TRB IDs
#define TRANSFER_EVENT                  32
#define COMMAND_COMPLETION_EVENT        33
#define PORT_STATUS_CHANGE_EVENT        34
#define BANDWIDTH_RESET_REQUEST_EVENT   35
#define DOORBELL_EVENT                  36
#define HOST_CONTROLLER_EVENT           37
#define DEVICE_NOTIFICATION_EVENT       38
#define MF_INDEX_WARP_EVENT             39


// TRB COMPLETION CODES
#define INVALID                     0
#define SUCCESS                     1
#define DATA_BUFFER_ERROR           2
#define BABBLLE_DETECTED_ERROR      3
#define USB_TRNASACTION_ERROR       4
#define TRB_ERROR                   5
#define STALL_ERROR                 6
#define RESOURCE_ERROR              7
#define BANDWIDTH_ERROR             8
#define NO_SLOTS_AVAILABLE_ERROR    9
#define INVALID_STREAM_TYPE_ERROR   10
#define SLOT_NOT_ENABLED_ERROR      11
#define ENDPOINT_NOT_ENABLED_ERROR  12
#define SHORT_PACKET                13
#define RING_UNDERRUN               14
#define RING_OVERRUN                15
#define VF_EVENT_RING_FULL_ERROR    16
#define PARAMETER_ERROR             17
#define BANDWIDTH_OVERRUN_ERROR     18
#define CONTEXT_STATE_ERROR         19
#define NO_PING_RESPONSE_ERROR      20
#define EVENT_RING_FULL_ERROR       21
#define INCOMPATIBLE_DEVICE_ERROR   22
#define MISSED_SERVICE_ERROR        23
#define COMMAND_RING_STOPPED        24
#define COMMAND_ABORTED             25
#define STOPPED                     26
#define STOPPED_LENGTH_INVALID      27
#define STOPPED_SHORT_PACKET        28
#define MAX_EXIT_LATENCY_ERROR      29
#define ISOCH_BUFFER_OVERRUN        31
#define EVENT_LOST_ERROR            32
#define UNDEFINED_ERROR             33
#define INVALID_STREAM_ID_ERROR     34
#define SECONDARY_BANDWIDTH_ERROR   35
#define SPLIT_TRNASACTION_ERROR     36

//Data structures
typedef struct  _XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY 
{
    PHYSICAL_ADDRESS ContextBaseAddr [256];
} XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY, *PXHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY;

//----------------------------------------LINK TRB--------------------------------------------------------------------
typedef union _XHCI_LINK_ADDR
{
    struct 
    {
        ULONGLONG RsvdZ1                     : 4;
        ULONGLONG RingSegmentPointerLo       : 28;
        ULONGLONG RingSegmentPointerHi       : 32;
    };
    ULONGLONG AsULONGLONG;
} XHCI_LINK_ADDR;

typedef struct _XHCI_LINK_TRB
{
    struct 
    {
        ULONG RsvdZ1                     : 4;
        ULONG RingSegmentPointerLo       : 28;
    };
    struct 
    {
        ULONG RingSegmentPointerHi       : 32;
    };
    struct 
    {
        ULONG RsvdZ2                     : 22;
        ULONG InterrupterTarget          : 10;
    };
    struct 
    {
        ULONG CycleBit                  : 1;
        ULONG ToggleCycle               : 1;
        ULONG RsvdZ3                    : 2;
        ULONG ChainBit                  : 1;
        ULONG InterruptOnCompletion     : 1;
        ULONG RsvdZ4                    : 4;
        ULONG TRBType                   : 6;
        ULONG RsvdZ5                    : 16;
    };
    //ULONG AsULONG;
} XHCI_LINK_TRB;
C_ASSERT(sizeof(XHCI_LINK_TRB) == 16);

//----------------------------------------generic trb----------------------------------------------------------------
typedef struct _XHCI_GENERIC_TRB {
    ULONG Word0;
    ULONG Word1;
    ULONG Word2;
    ULONG Word3;
}XHCI_GENERIC_TRB, *PXHCI_GENERIC_TRB;
C_ASSERT(sizeof(XHCI_GENERIC_TRB) == 16);
//----------------------------------------Command TRBs----------------------------------------------------------------
typedef struct _XHCI_COMMAND_NO_OP_TRB 
{
    ULONG RsvdZ1;
    ULONG RsvdZ2;
    ULONG RsvdZ3;
    struct
    {
        ULONG CycleBit                  : 1;
        ULONG RsvdZ4                    : 9;
        ULONG TRBType                   : 6;
        ULONG RsvdZ5                    : 16;
    };
} XHCI_COMMAND_NO_OP_TRB;
C_ASSERT(sizeof(XHCI_COMMAND_NO_OP_TRB) == 16);

typedef union _XHCI_COMMAND_TRB 
{
    XHCI_COMMAND_NO_OP_TRB NoOperation;
}XHCI_COMMAND_TRB, *PXHCI_COMMAND_TRB;
C_ASSERT(sizeof(XHCI_COMMAND_TRB) == 16);

//----------------------------------------CONTROL TRANSFER DATA STRUCTUERS--------------------------------------------
typedef struct _XHCI_CONTROL_SETUP_TRB 
{
    struct 
    {
        ULONG bmRequestType             : 8;
        ULONG bRequest                  : 8;
        ULONG wValue                    : 16;
    };
    struct 
    {
        ULONG wIndex                    : 16;
        ULONG wLength                   : 16;
    };
    struct 
    {
        ULONG TRBTransferLength         : 17;
        ULONG RsvdZ                     : 5;
        ULONG InterrupterTarget         : 10;
    };
    struct 
    {
        ULONG CycleBit                  : 1;
        ULONG RsvdZ1                    : 4;
        ULONG InterruptOnCompletion     : 1;
        ULONG ImmediateData             : 1;
        ULONG RsvdZ2                    : 3;
        ULONG TRBType                   : 6;
        ULONG TransferType              : 2;
        ULONG RsvdZ3                    : 14;
    };
} XHCI_CONTROL_SETUP_TRB;
C_ASSERT(sizeof(XHCI_CONTROL_SETUP_TRB) == 16);

typedef struct _XHCI_CONTROL_DATA_TRB 
{
    struct 
    {
        ULONG DataBufferPointerLo       : 32;
    };
    struct 
    {
        ULONG DataBufferPointerHi       : 32;
    };
    struct 
    {
        ULONG TRBTransferLength         : 17;
        ULONG TDSize                    : 5;
        ULONG InterrupterTarget         : 10;
    };
    struct 
    {
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
} XHCI_CONTROL_DATA_TRB;
C_ASSERT(sizeof(XHCI_CONTROL_DATA_TRB) == 16);

typedef struct _XHCI_CONTROL_STATUS_TRB 
{
    struct 
    {
        ULONG RsvdZ1                    : 32;
    };
    struct 
    {
        ULONG RsvdZ2                    : 32;
    };
    struct 
    {
        ULONG RsvdZ                     : 22;
        ULONG InterrupterTarget         : 10;
    };
    struct 
    {
        ULONG CycleBit                  : 1;
        ULONG EvaluateNextTRB           : 1;
        ULONG ChainBit                  : 2;
        ULONG InterruptOnCompletion     : 1;
        ULONG RsvdZ3                    : 4;
        ULONG TRBType                   : 6;
        ULONG Direction                 : 1;
        ULONG RsvdZ4                    : 15;
    };
} XHCI_CONTROL_STATUS_TRB;
C_ASSERT(sizeof(XHCI_CONTROL_STATUS_TRB) == 16);

typedef union _XHCI_CONTROL_TRB 
{
    XHCI_CONTROL_SETUP_TRB  SetupTRB;
    XHCI_CONTROL_DATA_TRB   DataTRB;
    XHCI_CONTROL_STATUS_TRB StatusTRB;
    XHCI_GENERIC_TRB    GenericTRB;
} XHCI_CONTROL_TRB, *PXHCI_CONTROL_TRB;  
C_ASSERT(sizeof(XHCI_CONTROL_TRB) == 16);

//----------------event strucs-------------------
typedef struct _XHCI_EVENT_COMMAND_COMPLETION_TRB
{
    struct 
    {
        ULONG RsvdZ1                : 4;
        ULONG CommandTRBPointerLo   : 28;
    };
    ULONG CommandTRBPointerHi;
    struct 
    {
        ULONG CommandCompletionParam     : 24;
        ULONG CompletionCode             : 8;
    };
    struct 
    {
        ULONG CycleBit          : 1;
        ULONG RsvdZ2            : 9;
        ULONG TRBType           : 6;
        ULONG VFID              : 8;
        ULONG SlotID            : 8;
    };
} XHCI_EVENT_COMMAND_COMPLETION_TRB;
C_ASSERT(sizeof(XHCI_EVENT_COMMAND_COMPLETION_TRB) == 16);

typedef struct _XHCI_EVENT_PORT_STATUS_CHANGE_TRB
{
    struct 
    {
        ULONG RsvdZ1                : 24;
        ULONG PortID                : 8;
    };
    ULONG RsvdZ2;
    struct 
    {
        ULONG RsvdZ3                     : 24;
        ULONG CompletionCode             : 8;
    };
    struct 
    {
        ULONG CycleBit          : 1;
        ULONG RsvdZ4            : 9;
        ULONG TRBType           : 6;
        ULONG RsvdZ5            : 16;
    };
} XHCI_EVENT_PORT_STATUS_CHANGE_TRB;
C_ASSERT(sizeof(XHCI_EVENT_PORT_STATUS_CHANGE_TRB) == 16);

typedef struct _XHCI_EVENT_GENERIC_TRB
{
    ULONG Word0;
    ULONG Word1;
    ULONG Word2;
    struct 
    {
        ULONG CycleBit          : 1;
        ULONG RsvdZ1            : 9;
        ULONG TRBType           : 6;
        ULONG RsvdZ2            : 8;
        ULONG SlotID            : 8;
    };
}XHCI_EVENT_GENERIC_TRB;
C_ASSERT(sizeof(XHCI_EVENT_GENERIC_TRB) == 16);

typedef union _XHCI_EVENT_TRB 
{
    XHCI_EVENT_COMMAND_COMPLETION_TRB   CommandCompletionTRB;
    XHCI_EVENT_PORT_STATUS_CHANGE_TRB   PortStatusChangeTRB;
    XHCI_EVENT_GENERIC_TRB              EventGenericTRB;
}XHCI_EVENT_TRB, *PXHCI_EVENT_TRB;
C_ASSERT(sizeof(XHCI_EVENT_TRB) == 16);

typedef struct _XHCI_EVENT_RING_SEGMENT_TABLE
{
    ULONGLONG RingSegmentBaseAddr;
    struct 
    {
        ULONGLONG RingSegmentSize :  16;
        ULONGLONG RsvdZ           :  48;
    };
} XHCI_EVENT_RING_SEGMENT_TABLE;
//------------------------------------main structs-----------------------

typedef union _XHCI_TRB 
{
    XHCI_COMMAND_TRB    CommandTRB;
    XHCI_LINK_TRB       LinkTRB;
    XHCI_CONTROL_TRB    ControlTRB;
    XHCI_EVENT_TRB      EventTRB;
    XHCI_GENERIC_TRB    GenericTRB;
} XHCI_TRB, *PXHCI_TRB;
C_ASSERT(sizeof(XHCI_TRB) == 16);

typedef struct _XHCI_SEGMENT 
{
    XHCI_TRB XhciTrb[256];
    PVOID nextSegment;
}XHCI_SEGMENT, *PXHCI_SEGMENT;

typedef struct _XHCI_RING 
{
    XHCI_SEGMENT firstSeg;
    PXHCI_TRB dequeue_pointer;
    PXHCI_TRB enqueue_pointer;
    PXHCI_SEGMENT enqueue_segment;
    PXHCI_SEGMENT dequeue_segment;
    struct 
    {
        UCHAR ProducerCycleState : 1;
        UCHAR ConsumerCycleState : 1;
    };
} XHCI_RING, *PXHCI_RING;

typedef struct _XHCI_HC_RESOURCES 
{
    XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY DCBAA;
    DECLSPEC_ALIGN(16) XHCI_RING         EventRing ;
    DECLSPEC_ALIGN(64) XHCI_RING         CommandRing ;
    DECLSPEC_ALIGN(64) XHCI_EVENT_RING_SEGMENT_TABLE EventRingSegTable;
} XHCI_HC_RESOURCES, *PXHCI_HC_RESOURCES;
C_ASSERT (FIELD_OFFSET(XHCI_HC_RESOURCES,EventRing)% 16 == 0); 
C_ASSERT (FIELD_OFFSET(XHCI_HC_RESOURCES,CommandRing)% 64 == 0); 
C_ASSERT (FIELD_OFFSET(XHCI_HC_RESOURCES,EventRingSegTable)% 64 == 0);

typedef struct _XHCI_EXTENSION 
{
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
    USHORT PageSize;
    USHORT MaxScratchPadBuffers;
    PMDL ScratchPadArrayMDL;
    PMDL ScratchPadBufferMDL;
    PXHCI_HC_RESOURCES HcResourcesVA;
    PHYSICAL_ADDRESS HcResourcesPA;
} XHCI_EXTENSION, *PXHCI_EXTENSION;


typedef struct _XHCI_ENDPOINT 
{
    ULONG Reserved;
} XHCI_ENDPOINT, *PXHCI_ENDPOINT;

typedef struct _XHCI_TRANSFER 
{
    ULONG Reserved;
} XHCI_TRANSFER, *PXHCI_TRANSFER;

typedef union _XHCI_SCRATCHPAD_BUFFER_ARRAY
{
    struct 
    {
        ULONGLONG RsvdZ1              :  12;
        ULONGLONG bufferBaseAddr      :  52;
    };
    ULONGLONG AsULONGLONG;
} XHCI_SCRATCHPAD_BUFFER_ARRAY, *PXHCI_SCRATCHPAD_BUFFER_ARRAY;
C_ASSERT(sizeof(XHCI_SCRATCHPAD_BUFFER_ARRAY) == 8);
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