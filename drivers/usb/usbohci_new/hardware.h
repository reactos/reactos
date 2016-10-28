#define OHCI_NUMBER_OF_INTERRUPTS  32
#define OHCI_MAX_PORT_COUNT        15

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

typedef struct _OHCI_TRANSFER_DESCRIPTOR { // must be aligned to a 16-byte boundary
  OHCI_TRANSFER_CONTROL Control;
  PVOID CurrentBuffer; // physical address of the next memory location
  PULONG NextTD; // pointer to the next TD on the list of TDs
  PVOID BufferEnd; // physical address of the last byte
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

typedef struct _OHCI_ISO_TRANSFER_DESCRIPTOR { // must be aligned to a 32-byte boundary
  OHCI_ISO_TRANSFER_CONTROL Control;
  PVOID BufferPage0; // physical page number of the 1 byte of the data buffer
  PULONG NextTD; // pointer to the next Isochronous TD on the queue of Isochronous TDs
  PVOID BufferEnd; // physical address of the last byte in the buffer
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

typedef struct _OHCI_ENDPOINT_DESCRIPTOR { // must be aligned to a 16-byte boundary
  OHCI_ENDPOINT_CONTROL EndpointControl;
  ULONG_PTR TailPointer; // if TailP and HeadP are different, then the list contains a TD to be processed
  ULONG_PTR HeadPointer; // physical pointer to the next TD to be processed for this endpoint
  ULONG_PTR NextED; // entry points to the next ED on the list
} OHCI_ENDPOINT_DESCRIPTOR, *POHCI_ENDPOINT_DESCRIPTOR;

C_ASSERT(sizeof(OHCI_ENDPOINT_DESCRIPTOR) == 16);

typedef struct _OHCI_HCCA { // must be located on a 256-byte boundary
  POHCI_ENDPOINT_DESCRIPTOR InterrruptTable[OHCI_NUMBER_OF_INTERRUPTS];
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
    ULONG Reserved2           : 1; //NULL
  };
  ULONG AsULONG;
} OHCI_REG_INTERRUPT_STATUS, *POHCI_REG_INTERRUPT_STATUS;

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

typedef union _OHCI_REG_FRAME_INTERVAL {
  struct {
    ULONG FrameInterval       : 14;
    ULONG Reserved            : 2;
    ULONG FSLargestDataPacket : 15;
    ULONG FrameIntervalToggle : 1;
  };
  ULONG AsULONG;
} OHCI_REG_FRAME_INTERVAL, *POHCI_REG_FRAME_INTERVAL;

typedef union _OHCI_REG_RH_DESCRIPTORA {
  struct {
    ULONG NumberDownstreamPorts     : 8;
    ULONG PowerSwitchingMode        : 1;
    ULONG NoPowerSwitching          : 1;
    ULONG DeviceType                : 1;
    ULONG OverCurrentProtectionMode : 1;
    ULONG NoOverCurrentProtection   : 1;
    ULONG Reserved2                 : 13;
    ULONG PowerOnToPowerGoodTime    : 1;
  };
  ULONG AsULONG;
} OHCI_REG_RH_DESCRIPTORA, *POHCI_REG_RH_DESCRIPTORA;

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

typedef union _OHCI_REG_RH_PORT_STATUS {
  struct {
    union  { // 0 byte
      // read
      UCHAR  CurrentConnectStatus     : 1;
      UCHAR  PortEnableStatus         : 1;
      UCHAR  PortSuspendStatus        : 1;
      UCHAR  PortOverCurrentIndicator : 1;
      UCHAR  PortResetStatus          : 1;
      UCHAR  Reserved1r               : 3;
      // write
      UCHAR  ClearPortEnable    : 1;
      UCHAR  SetPortEnable      : 1;
      UCHAR  SetPortSuspend     : 1;
      UCHAR  ClearSuspendStatus : 1;
      UCHAR  SetPortReset       : 1;
      UCHAR  Reserved1w         : 3;
    };
    union  { // 1 byte
      // read
      UCHAR  PortPowerStatus        : 1;
      UCHAR  LowSpeedDeviceAttached : 1;
      UCHAR  Reserved2r             : 6;
      // write
      UCHAR  SetPortPower           : 1;
      UCHAR  ClearPortPower         : 1;
      UCHAR  Reserved2w             : 6;
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

typedef struct _OHCI_OPERATIONAL_REGISTERS {
  ULONG HcRevision;
  OHCI_REG_CONTROL HcControl; // +4
  OHCI_REG_COMMAND_STATUS HcCommandStatus; // +8
  OHCI_REG_INTERRUPT_STATUS HcInterruptStatus; // +12 0x0C
  OHCI_REG_INTERRUPT_ENABLE_DISABLE HcInterruptEnable; // +16 0x10
  OHCI_REG_INTERRUPT_ENABLE_DISABLE HcInterruptDisable; // +20 0x14
  ULONG HcHCCA; // +24 0x18
  ULONG HcPeriodCurrentED; // +28 0x1C
  ULONG HcControlHeadED; // +32 0x20
  ULONG HcControlCurrentED; // +36 0x24
  ULONG HcBulkHeadED; // +40 0x28
  ULONG HcBulkCurrentED; // +44 0x2C
  ULONG HcDoneHead; // +48 0x30
  OHCI_REG_FRAME_INTERVAL HcFmInterval; // +52 0x34
  ULONG HcFmRemaining; // +56 0x38
  ULONG HcFmNumber; // +60 0x3C
  ULONG HcPeriodicStart; // +64 0x40
  ULONG HcLSThreshold; // +68 0x44
  OHCI_REG_RH_DESCRIPTORA HcRhDescriptorA; // +72 0x48
  ULONG HcRhDescriptorB; // +76 0x4C
  OHCI_REG_RH_STATUS HcRhStatus; // +80 0x50
  OHCI_REG_RH_PORT_STATUS HcRhPortStatus[OHCI_MAX_PORT_COUNT]; // +84 0x54 ... 144 0x90
} OHCI_OPERATIONAL_REGISTERS, *POHCI_OPERATIONAL_REGISTERS;
