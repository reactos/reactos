/* EHCI hardware registers */

typedef union _EHCI_HC_STRUCTURAL_PARAMS {
  struct {
    ULONG PortCount            : 4;
    ULONG PortPowerControl     : 1;
    ULONG Reserved1            : 2;
    ULONG PortRouteRules       : 1;
    ULONG PortsPerCompanion    : 4;
    ULONG CompanionControllers : 4;
    ULONG PortIndicators       : 1;
    ULONG Reserved2            : 3;
    ULONG DebugPortNumber      : 4; //Optional
    ULONG Reserved3            : 8;
  };
  ULONG AsULONG;
} EHCI_HC_STRUCTURAL_PARAMS;

typedef union _EHCI_HC_CAPABILITY_PARAMS {
  struct {
    ULONG Addressing64bitCapability : 1;
    ULONG IsProgrammableFrameList   : 1;
    ULONG IsScheduleParkSupport     : 1;
    ULONG Reserved1                 : 1;
    ULONG IsoSchedulingThreshold    : 4;
    ULONG ExtCapabilitiesPointer    : 8; // (EECP)
    ULONG Reserved2                 : 16;
  };
  ULONG AsULONG;
} EHCI_HC_CAPABILITY_PARAMS;

typedef union _EHCI_USB_COMMAND {
  struct {
    ULONG Run                        : 1;
    ULONG Reset                      : 1;
    ULONG FrameListSize              : 2;
    ULONG PeriodicEnable             : 1;
    ULONG AsynchronousEnable         : 1;
    ULONG InterruptAdvanceDoorbell   : 1;
    ULONG LightResetHC               : 1; // optional
    ULONG AsynchronousParkModeCount  : 2; // optional
    ULONG Reserved1                  : 1;
    ULONG AsynchronousParkModeEnable : 1; // optional
    ULONG Reserved2                  : 4;
    ULONG InterruptThreshold         : 8;
    ULONG Reserved3                  : 8;
  };
  ULONG AsULONG;
} EHCI_USB_COMMAND;

typedef union _EHCI_USB_STATUS {
  struct {
    ULONG Interrupt               : 1;
    ULONG ErrorInterrupt          : 1;
    ULONG PortChangeDetect        : 1;
    ULONG FrameListRollover       : 1;
    ULONG HostSystemError         : 1;
    ULONG InterruptOnAsyncAdvance : 1;
    ULONG Reserved1               : 6;
    ULONG HCHalted                : 1;
    ULONG Reclamation             : 1;
    ULONG PeriodicStatus          : 1;
    ULONG AsynchronousStatus      : 1;
    ULONG Reserved2               : 16;
  };
  ULONG AsULONG;
} EHCI_USB_STATUS;

typedef union _EHCI_INTERRUPT_ENABLE {
  struct {
    ULONG Interrupt               : 1;
    ULONG ErrorInterrupt          : 1;
    ULONG PortChangeInterrupt     : 1;
    ULONG FrameListRollover       : 1;
    ULONG HostSystemError         : 1;
    ULONG InterruptOnAsyncAdvance : 1;
    ULONG Reserved                : 26;
  };
  ULONG AsULONG;
} EHCI_INTERRUPT_ENABLE;

typedef union _EHCI_PORT_STATUS_CONTROL {
  struct {
    ULONG CurrentConnectStatus    : 1;
    ULONG ConnectStatusChange     : 1;
    ULONG PortEnabledDisabled     : 1;
    ULONG PortEnableDisableChange : 1;
    ULONG OverCurrentActive       : 1;
    ULONG OverCurrentChange       : 1;
    ULONG ForcePortResume         : 1;
    ULONG Suspend                 : 1;
    ULONG PortReset               : 1;
    ULONG Reserved1               : 1;
    ULONG LineStatus              : 2;
    ULONG PortPower               : 1;
    ULONG PortOwner               : 1;
    ULONG PortIndicatorControl    : 2;
    ULONG PortTestControl         : 4;
    ULONG WakeOnConnectEnable     : 1;
    ULONG WakeOnDisconnectEnable  : 1;
    ULONG WakeOnOverCurrentEnable : 1;
    ULONG Reserved2               : 9;
  };
  ULONG AsULONG;
} EHCI_PORT_STATUS_CONTROL;

/* Link Pointer */

#define EHCI_LINK_TYPE_iTD  0 // isochronous transfer descriptor
#define EHCI_LINK_TYPE_QH   1 // queue head
#define EHCI_LINK_TYPE_siTD 2 // split transaction isochronous transfer
#define EHCI_LINK_TYPE_FSTN 3 // frame span traversal node

typedef union _EHCI_LINK_POINTER {
  struct {
    ULONG Terminate : 1;
    ULONG Type      : 2;
    ULONG Reserved  : 2;
    ULONG Adress    : 27;
  };
  ULONG AsULONG;
} EHCI_LINK_POINTER;

/* Isochronous (High-Speed) Transfer Descriptor (iTD) */

typedef union _EHCI_TRANSACTION_CONTROL {
  struct {
    ULONG xOffset             : 12;
    ULONG PageSelect          : 3;
    ULONG InterruptOnComplete : 1;
    ULONG xLength             : 12;
    ULONG Status              : 4;
  };
  ULONG AsULONG;
} EHCI_TRANSACTION_CONTROL;

typedef union _EHCI_TRANSACTION_BUFFER {
  struct {
    ULONG DeviceAddress  : 7;
    ULONG Reserved1      : 1;
    ULONG EndpointNumber : 4;
    ULONG DataBuffer0    : 20;
  };
  struct {
    ULONG MaximumPacketSize : 11;
    ULONG Direction         : 1;
    ULONG DataBuffer1       : 20;
  };
  struct {
    ULONG Multi       : 2;
    ULONG Reserved2   : 10;
    ULONG DataBuffer2 : 20;
  };
  struct {
    ULONG Reserved3  : 12;
    ULONG DataBuffer : 20;
  };
  ULONG AsULONG;
} EHCI_TRANSACTION_BUFFER;

typedef struct _EHCI_ISOCHRONOUS_TD { // must be aligned on a 32-byte boundary
  EHCI_LINK_POINTER NextLink;
  EHCI_TRANSACTION_CONTROL Transaction[8];
  EHCI_TRANSACTION_BUFFER Buffer[7];
  ULONG_PTR ExtendedBuffer[7];
} EHCI_ISOCHRONOUS_TD, *PEHCI_ISOCHRONOUS_TD;

C_ASSERT(sizeof(EHCI_ISOCHRONOUS_TD) == 92);
