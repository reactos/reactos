/*
 * PROJECT:     ReactOS USB OHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBOHCI hardware declarations
 * COPYRIGHT:   Copyright 2017-2018 Vadim Galyant <vgal@rambler.ru>
 */

#define OHCI_NUMBER_OF_INTERRUPTS    32
#define OHCI_MAX_PORT_COUNT          15
#define ED_EOF                       -1
#define OHCI_MAXIMUM_OVERHEAD        210 // 5.4  FrameInterval Counter, in bit-times
#define OHCI_DEFAULT_FRAME_INTERVAL  11999 // 6.3.1  Frame Timing
#define OHCI_MINIMAL_POTPGT          25 // == 50 ms., PowerOnToPowerGoodTime (HcRhDescriptorA Register)

/* Controller states */
#define OHCI_HC_STATE_RESET       0
#define OHCI_HC_STATE_RESUME      1
#define OHCI_HC_STATE_OPERATIONAL 2
#define OHCI_HC_STATE_SUSPEND     3

/* Endpoint Descriptor Control */
#define OHCI_ED_DATA_FLOW_DIRECTION_FROM_TD 0
#define OHCI_ED_DATA_FLOW_DIRECTION_OUT     1
#define OHCI_ED_DATA_FLOW_DIRECTION_IN      2

#define OHCI_ENDPOINT_FULL_SPEED 0
#define OHCI_ENDPOINT_LOW_SPEED  1

#define OHCI_ENDPOINT_GENERAL_FORMAT     0
#define OHCI_ENDPOINT_ISOCHRONOUS_FORMAT 1

/* Transfer Descriptor Control */
#define OHCI_TD_INTERRUPT_IMMEDIATE 0
#define OHCI_TD_INTERRUPT_NONE      7

#define OHCI_TD_DIRECTION_PID_SETUP    0
#define OHCI_TD_DIRECTION_PID_OUT      1
#define OHCI_TD_DIRECTION_PID_IN       2
#define OHCI_TD_DIRECTION_PID_RESERVED 3

#define OHCI_TD_DATA_TOGGLE_FROM_ED  0
#define OHCI_TD_DATA_TOGGLE_DATA0    2
#define OHCI_TD_DATA_TOGGLE_DATA1    3

#define OHCI_TD_CONDITION_NO_ERROR          0x00
#define OHCI_TD_CONDITION_CRC_ERROR         0x01
#define OHCI_TD_CONDITION_BIT_STUFFING      0x02
#define OHCI_TD_CONDITION_TOGGLE_MISMATCH   0x03
#define OHCI_TD_CONDITION_STALL             0x04
#define OHCI_TD_CONDITION_NO_RESPONSE       0x05
#define OHCI_TD_CONDITION_PID_CHECK_FAILURE 0x06
#define OHCI_TD_CONDITION_UNEXPECTED_PID    0x07
#define OHCI_TD_CONDITION_DATA_OVERRUN      0x08
#define OHCI_TD_CONDITION_DATA_UNDERRUN     0x09
#define OHCI_TD_CONDITION_BUFFER_OVERRUN    0x0C
#define OHCI_TD_CONDITION_BUFFER_UNDERRUN   0x0D
#define OHCI_TD_CONDITION_NOT_ACCESSED      0x0E

typedef union _OHCI_TRANSFER_CONTROL {
  struct {
    ULONG Reserved       : 18;
    ULONG BufferRounding : 1;
    ULONG DirectionPID   : 2;
    ULONG DelayInterrupt : 3;
    ULONG DataToggle     : 2;
    ULONG ErrorCount     : 2;
    ULONG ConditionCode  : 4;
  };
  ULONG  AsULONG;
} OHCI_TRANSFER_CONTROL, *POHCI_TRANSFER_CONTROL;

C_ASSERT(sizeof(OHCI_TRANSFER_CONTROL) == sizeof(ULONG));

typedef struct _OHCI_TRANSFER_DESCRIPTOR { // must be aligned to a 16-byte boundary
  OHCI_TRANSFER_CONTROL Control;
  ULONG CurrentBuffer; // physical address of the next memory location
  ULONG NextTD; // pointer to the next TD on the list of TDs
  ULONG BufferEnd; // physical address of the last byte
} OHCI_TRANSFER_DESCRIPTOR, *POHCI_TRANSFER_DESCRIPTOR;

C_ASSERT(sizeof(OHCI_TRANSFER_DESCRIPTOR) == 16);

typedef union _OHCI_ISO_TRANSFER_CONTROL {
  struct {
    ULONG StartingFrame  : 16;
    ULONG Reserved1      : 5;
    ULONG DelayInterrupt : 3;
    ULONG FrameCount     : 3;
    ULONG Reserved2      : 1;
    ULONG ConditionCode  : 4;
  };
  ULONG  AsULONG;
} OHCI_ISO_TRANSFER_CONTROL, *POHCI_ISO_TRANSFER_CONTROL;

C_ASSERT(sizeof(OHCI_ISO_TRANSFER_CONTROL) == sizeof(ULONG));

typedef struct _OHCI_ISO_TRANSFER_DESCRIPTOR { // must be aligned to a 32-byte boundary
  OHCI_ISO_TRANSFER_CONTROL Control;
  ULONG BufferPage0; // physical page number of the 1 byte of the data buffer
  ULONG NextTD; // pointer to the next Isochronous TD on the queue of Isochronous TDs
  ULONG BufferEnd; // physical address of the last byte in the buffer
  USHORT Offset[8]; // for determine size and start addr. iso packet | PacketStatusWord - completion code
} OHCI_ISO_TRANSFER_DESCRIPTOR, *POHCI_ISO_TRANSFER_DESCRIPTOR;

C_ASSERT(sizeof(OHCI_ISO_TRANSFER_DESCRIPTOR) == 32);

typedef union _OHCI_ENDPOINT_CONTROL {
  struct {
    ULONG FunctionAddress   : 7;
    ULONG EndpointNumber    : 4;
    ULONG Direction         : 2;
    ULONG Speed             : 1;
    ULONG sKip              : 1;
    ULONG Format            : 1;
    ULONG MaximumPacketSize : 11;
    ULONG Reserved          : 5;
  };
  ULONG  AsULONG;
} OHCI_ENDPOINT_CONTROL, *POHCI_ENDPOINT_CONTROL;

C_ASSERT(sizeof(OHCI_ENDPOINT_CONTROL) == sizeof(ULONG));

/* Bit flags for HeadPointer member of the EP descriptor */
#define OHCI_ED_HEAD_POINTER_HALT        0x00000001 // hardware stopped bit
#define OHCI_ED_HEAD_POINTER_CARRY       0x00000002 // hardware toggle carry bit
#define OHCI_ED_HEAD_POINTER_MASK        0XFFFFFFF0 // mask physical pointer
#define OHCI_ED_HEAD_POINTER_FLAGS_MASK  0X0000000F // mask bit flags

typedef struct _OHCI_ENDPOINT_DESCRIPTOR { // must be aligned to a 16-byte boundary
  OHCI_ENDPOINT_CONTROL EndpointControl;
  ULONG TailPointer; // if TailP and HeadP are different, then the list contains a TD to be processed
  ULONG HeadPointer; // physical pointer to the next TD to be processed for this endpoint
  ULONG NextED; // entry points to the next ED on the list
} OHCI_ENDPOINT_DESCRIPTOR, *POHCI_ENDPOINT_DESCRIPTOR;

C_ASSERT(sizeof(OHCI_ENDPOINT_DESCRIPTOR) == 16);

typedef struct _OHCI_HCCA { // must be located on a 256-byte boundary
  ULONG InterrruptTable[OHCI_NUMBER_OF_INTERRUPTS];
  USHORT FrameNumber;
  USHORT Pad1;
  ULONG DoneHead;
  UCHAR reserved_hc[116];
  UCHAR Pad[4];
} OHCI_HCCA, *POHCI_HCCA;

C_ASSERT(sizeof(OHCI_HCCA) == 256);

typedef union _OHCI_REG_CONTROL {
  struct {
    ULONG ControlBulkServiceRatio       : 2;
    ULONG PeriodicListEnable            : 1;
    ULONG IsochronousEnable             : 1;
    ULONG ControlListEnable             : 1;
    ULONG BulkListEnable                : 1;
    ULONG HostControllerFunctionalState : 2;
    ULONG InterruptRouting              : 1;
    ULONG RemoteWakeupConnected         : 1;
    ULONG RemoteWakeupEnable            : 1;
    ULONG Reserved                      : 21;
  };
  ULONG AsULONG;
} OHCI_REG_CONTROL, *POHCI_REG_CONTROL;

C_ASSERT(sizeof(OHCI_REG_CONTROL) == sizeof(ULONG));

typedef union _OHCI_REG_COMMAND_STATUS {
  struct {
    ULONG HostControllerReset    : 1;
    ULONG ControlListFilled      : 1;
    ULONG BulkListFilled         : 1;
    ULONG OwnershipChangeRequest : 1;
    ULONG Reserved1              : 12;
    ULONG SchedulingOverrunCount : 1;
    ULONG Reserved2              : 15;
  };
  ULONG AsULONG;
} OHCI_REG_COMMAND_STATUS, *POHCI_REG_COMMAND_STATUS;

C_ASSERT(sizeof(OHCI_REG_COMMAND_STATUS) == sizeof(ULONG));

typedef union _OHCI_REG_INTERRUPT_STATUS {
  struct {
    ULONG SchedulingOverrun   : 1;
    ULONG WritebackDoneHead   : 1;
    ULONG StartofFrame        : 1;
    ULONG ResumeDetected      : 1;
    ULONG UnrecoverableError  : 1;
    ULONG FrameNumberOverflow : 1;
    ULONG RootHubStatusChange : 1;
    ULONG Reserved1           : 23;
    ULONG OwnershipChange     : 1;
    ULONG Reserved2           : 1;
  };
  ULONG AsULONG;
} OHCI_REG_INTERRUPT_STATUS, *POHCI_REG_INTERRUPT_STATUS;

C_ASSERT(sizeof(OHCI_REG_INTERRUPT_STATUS) == sizeof(ULONG));

typedef union _OHCI_REG_INTERRUPT_ENABLE_DISABLE {
  struct {
    ULONG SchedulingOverrun     : 1;
    ULONG WritebackDoneHead     : 1;
    ULONG StartofFrame          : 1;
    ULONG ResumeDetected        : 1;
    ULONG UnrecoverableError    : 1;
    ULONG FrameNumberOverflow   : 1;
    ULONG RootHubStatusChange   : 1;
    ULONG Reserved1             : 23;
    ULONG OwnershipChange       : 1;
    ULONG MasterInterruptEnable : 1;
  };
  ULONG AsULONG;
} OHCI_REG_INTERRUPT_ENABLE_DISABLE, *POHCI_REG_INTERRUPT_ENABLE_DISABLE;

C_ASSERT(sizeof(OHCI_REG_INTERRUPT_ENABLE_DISABLE) == sizeof(ULONG));

typedef union _OHCI_REG_FRAME_INTERVAL {
  struct {
    ULONG FrameInterval       : 14;
    ULONG Reserved            : 2;
    ULONG FSLargestDataPacket : 15;
    ULONG FrameIntervalToggle : 1;
  };
  ULONG AsULONG;
} OHCI_REG_FRAME_INTERVAL, *POHCI_REG_FRAME_INTERVAL;

C_ASSERT(sizeof(OHCI_REG_FRAME_INTERVAL) == sizeof(ULONG));

typedef union _OHCI_REG_RH_DESCRIPTORA {
  struct {
    ULONG NumberDownstreamPorts     : 8;
    ULONG PowerSwitchingMode        : 1;
    ULONG NoPowerSwitching          : 1;
    ULONG DeviceType                : 1;
    ULONG OverCurrentProtectionMode : 1;
    ULONG NoOverCurrentProtection   : 1;
    ULONG Reserved                  : 11;
    ULONG PowerOnToPowerGoodTime    : 8;
  };
  ULONG AsULONG;
} OHCI_REG_RH_DESCRIPTORA, *POHCI_REG_RH_DESCRIPTORA;

C_ASSERT(sizeof(OHCI_REG_RH_DESCRIPTORA) == sizeof(ULONG));

typedef union _OHCI_REG_RH_STATUS {
  union {
    struct { // read
      ULONG LocalPowerStatus            : 1;
      ULONG OverCurrentIndicator        : 1;
      ULONG Reserved10                  : 13;
      ULONG DeviceRemoteWakeupEnable    : 1;
      ULONG LocalPowerStatusChange      : 1;
      ULONG OverCurrentIndicatorChangeR : 1;
      ULONG Reserved20                  : 14;
    };
    struct { // write
      ULONG ClearGlobalPower            : 1;
      ULONG Reserved11                  : 14;
      ULONG SetRemoteWakeupEnable       : 1;
      ULONG SetGlobalPower              : 1;
      ULONG OverCurrentIndicatorChangeW : 1;
      ULONG Reserved22                  : 13;
      ULONG ClearRemoteWakeupEnable     : 1;
    };
  };
  ULONG AsULONG;
} OHCI_REG_RH_STATUS, *POHCI_REG_RH_STATUS;

C_ASSERT(sizeof(OHCI_REG_RH_STATUS) == sizeof(ULONG));

typedef union _OHCI_REG_RH_PORT_STATUS {
  struct {
    union  {
      struct { // read
        USHORT  CurrentConnectStatus     : 1;
        USHORT  PortEnableStatus         : 1;
        USHORT  PortSuspendStatus        : 1;
        USHORT  PortOverCurrentIndicator : 1;
        USHORT  PortResetStatus          : 1;
        USHORT  Reserved1r               : 3;
        USHORT  PortPowerStatus          : 1;
        USHORT  LowSpeedDeviceAttached   : 1;
        USHORT  Reserved2r               : 6;
      };
      struct { // write
        USHORT  ClearPortEnable    : 1;
        USHORT  SetPortEnable      : 1;
        USHORT  SetPortSuspend     : 1;
        USHORT  ClearSuspendStatus : 1;
        USHORT  SetPortReset       : 1;
        USHORT  Reserved1w         : 3;
        USHORT  SetPortPower       : 1;
        USHORT  ClearPortPower     : 1;
        USHORT  Reserved2w         : 6;
      };
    };
    USHORT ConnectStatusChange            : 1;
    USHORT PortEnableStatusChange         : 1;
    USHORT PortSuspendStatusChange        : 1;
    USHORT PortOverCurrentIndicatorChange : 1;
    USHORT PortResetStatusChange          : 1;
    USHORT Reserved3                      : 11;
  };
  ULONG  AsULONG;
} OHCI_REG_RH_PORT_STATUS, *POHCI_REG_RH_PORT_STATUS;

C_ASSERT(sizeof(OHCI_REG_RH_PORT_STATUS) == sizeof(ULONG));

typedef struct _OHCI_OPERATIONAL_REGISTERS {
  ULONG HcRevision;
  OHCI_REG_CONTROL HcControl;
  OHCI_REG_COMMAND_STATUS HcCommandStatus;
  OHCI_REG_INTERRUPT_STATUS HcInterruptStatus;
  OHCI_REG_INTERRUPT_ENABLE_DISABLE HcInterruptEnable;
  OHCI_REG_INTERRUPT_ENABLE_DISABLE HcInterruptDisable;
  ULONG HcHCCA;
  ULONG HcPeriodCurrentED;
  ULONG HcControlHeadED;
  ULONG HcControlCurrentED;
  ULONG HcBulkHeadED;
  ULONG HcBulkCurrentED;
  ULONG HcDoneHead;
  OHCI_REG_FRAME_INTERVAL HcFmInterval;
  ULONG HcFmRemaining;
  ULONG HcFmNumber;
  ULONG HcPeriodicStart;
  ULONG HcLSThreshold;
  OHCI_REG_RH_DESCRIPTORA HcRhDescriptorA;
  ULONG HcRhDescriptorB;
  OHCI_REG_RH_STATUS HcRhStatus;
  OHCI_REG_RH_PORT_STATUS HcRhPortStatus[OHCI_MAX_PORT_COUNT];
} OHCI_OPERATIONAL_REGISTERS, *POHCI_OPERATIONAL_REGISTERS;
