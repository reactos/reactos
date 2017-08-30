#define EHCI_FRAME_LIST_MAX_ENTRIES  1024 // Number of frames in Frame List

/* EHCI hardware registers */
#define EHCI_USBCMD            0
#define EHCI_USBSTS            1
#define EHCI_USBINTR           2
#define EHCI_FRINDEX           3
#define EHCI_CTRLDSSEGMENT     4
#define EHCI_PERIODICLISTBASE  5
#define EHCI_ASYNCLISTBASE     6
#define EHCI_CONFIGFLAG        16
#define EHCI_PORTSC            17

typedef union _EHCI_LEGACY_EXTENDED_CAPABILITY {
  struct {
    ULONG CapabilityID            : 8;
    ULONG NextCapabilityPointer   : 8;
    ULONG BiosOwnedSemaphore      : 1;
    ULONG Reserved1               : 7;
    ULONG OsOwnedSemaphore        : 1;
    ULONG Reserved2               : 7;
  };
  ULONG AsULONG;
} EHCI_LEGACY_EXTENDED_CAPABILITY;

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

typedef struct _EHCI_HC_CAPABILITY_REGISTERS {
  UCHAR RegistersLength; // RO
  UCHAR Reserved; // RO
  USHORT InterfaceVersion; // RO
  EHCI_HC_STRUCTURAL_PARAMS StructParameters; // RO
  EHCI_HC_CAPABILITY_PARAMS CapParameters; // RO
  UCHAR CompanionPortRouteDesc[8]; // RO
} EHCI_HC_CAPABILITY_REGISTERS, *PEHCI_HC_CAPABILITY_REGISTERS;

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

typedef struct _EHCI_HW_REGISTERS {
  EHCI_USB_COMMAND HcCommand; // RO, R/W (field dependent), WO
  EHCI_USB_STATUS HcStatus; // RO, R/W, R/WC, (field dependent)
  EHCI_INTERRUPT_ENABLE HcInterruptEnable; // R/W
  ULONG FrameIndex; // R/W (Writes must be DWord Writes)
  ULONG SegmentSelector; // R/W (Writes must be DWord Writes)
  ULONG PeriodicListBase; // R/W (Writes must be DWord Writes)
  ULONG AsyncListBase; // Read/Write (Writes must be DWord Writes) 
  ULONG Reserved[9];
  ULONG ConfiFlag; // R/W
  EHCI_PORT_STATUS_CONTROL PortControl[15]; // (1-15) RO, R/W, R/WC (field dependent) 
} EHCI_HW_REGISTERS, *PEHCI_HW_REGISTERS;

/* Link Pointer */
#define EHCI_LINK_TYPE_iTD  0 // isochronous transfer descriptor
#define EHCI_LINK_TYPE_QH   1 // queue head
#define EHCI_LINK_TYPE_siTD 2 // split transaction isochronous transfer
#define EHCI_LINK_TYPE_FSTN 3 // frame span traversal node

/* Used for QHs and qTDs to mark Pointers as the end */
#define TERMINATE_POINTER   1

#define LINK_POINTER_MASK   0xFFFFFFE0

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

/* Split Transaction Isochronous Transfer Descriptor (siTD) */
typedef union _EHCI_FS_ENDPOINT_PARAMS {
  struct {
    ULONG DeviceAddress  : 7;
    ULONG Reserved1      : 1;
    ULONG EndpointNumber : 4;
    ULONG Reserved2      : 4;
    ULONG HubAddress     : 7;
    ULONG Reserved3      : 1;
    ULONG PortNumber     : 7;
    ULONG Direction      : 1;
  };
  ULONG AsULONG;
} EHCI_FS_ENDPOINT_PARAMS;

typedef union _EHCI_MICROFRAME_CONTROL {
  struct {
    ULONG StartMask      : 8;
    ULONG CompletionMask : 8;
    ULONG Reserved       : 16;
  };
  ULONG AsULONG;
} EHCI_MICROFRAME_CONTROL;

typedef union _EHCI_SPLIT_TRANSFER_STATE {
  struct {
    ULONG Status              : 8;
    ULONG ProgressMask        : 8;
    ULONG TotalBytes          : 10;
    ULONG Reserved            : 4;
    ULONG PageSelect          : 1;
    ULONG InterruptOnComplete : 1;
  };
  ULONG AsULONG;
} EHCI_SPLIT_TRANSFER_STATE;

typedef union _EHCI_SPLIT_BUFFER_POINTER {
  struct {
    ULONG CurrentOffset : 12;
    ULONG DataBuffer0   : 20;
  };
  struct {
    ULONG TransactionCount    : 3;
    ULONG TransactionPosition : 2;
    ULONG Reserved            : 7;
    ULONG DataBuffer1         : 20;
  };
  ULONG AsULONG;
} EHCI_SPLIT_BUFFER_POINTER;

typedef struct _EHCI_SPLIT_ISOCHRONOUS_TD { // must be aligned on a 32-byte boundary
  EHCI_LINK_POINTER NextLink;
  EHCI_FS_ENDPOINT_PARAMS EndpointCharacteristics;
  EHCI_MICROFRAME_CONTROL MicroFrameControl;
  EHCI_SPLIT_TRANSFER_STATE TransferState;
  EHCI_SPLIT_BUFFER_POINTER Buffer[2];
  ULONG_PTR BackPointer;
} EHCI_SPLIT_ISOCHRONOUS_TD, *PEHCI_SPLIT_ISOCHRONOUS_TD;

C_ASSERT(sizeof(EHCI_SPLIT_ISOCHRONOUS_TD) == 28);

/* Queue Element Transfer Descriptor (qTD) */
#define	EHCI_TOKEN_STATUS_ACTIVE            (1 << 7)
#define	EHCI_TOKEN_STATUS_HALTED            (1 << 6)
#define	EHCI_TOKEN_STATUS_DATA_BUFFER_ERROR (1 << 5)
#define	EHCI_TOKEN_STATUS_BABBLE_DETECTED   (1 << 4)
#define	EHCI_TOKEN_STATUS_TRANSACTION_ERROR (1 << 3)
#define	EHCI_TOKEN_STATUS_MISSED_MICROFRAME (1 << 2)
#define	EHCI_TOKEN_STATUS_SPLIT_STATE       (1 << 1)
#define	EHCI_TOKEN_STATUS_PING_STATE        (1 << 0)

typedef union _EHCI_TD_TOKEN {
  struct {
    ULONG Status              : 8;
    ULONG PIDCode             : 2;
    ULONG ErrorCounter        : 2;
    ULONG CurrentPage         : 3;
    ULONG InterruptOnComplete : 1;
    ULONG TransferBytes       : 15;
    ULONG DataToggle          : 1;
  };
  ULONG AsULONG;
} EHCI_TD_TOKEN, *PEHCI_TD_TOKEN;

typedef struct _EHCI_QUEUE_TD { // must be aligned on 32-byte boundaries
  ULONG_PTR NextTD;
  ULONG_PTR AlternateNextTD;
  EHCI_TD_TOKEN Token;
  ULONG_PTR Buffer[5];
  ULONG_PTR ExtendedBuffer[5];
} EHCI_QUEUE_TD, *PEHCI_QUEUE_TD;

C_ASSERT(sizeof(EHCI_QUEUE_TD) == 52);

/* Queue Head */
typedef union _EHCI_QH_EP_PARAMS {
  struct {
    ULONG DeviceAddress               : 7;
    ULONG InactivateOnNextTransaction : 1;
    ULONG EndpointNumber              : 4;
    ULONG EndpointSpeed               : 2;
    ULONG DataToggleControl           : 1;
    ULONG HeadReclamationListFlag     : 1;
    ULONG MaximumPacketLength         : 11; // corresponds to the maximum packet size of the associated endpoint (wMaxPacketSize).
    ULONG ControlEndpointFlag         : 1;
    ULONG NakCountReload              : 4;
  };
  ULONG AsULONG;
} EHCI_QH_EP_PARAMS;

typedef union _EHCI_QH_EP_CAPS {
  struct {
    ULONG InterruptMask       : 8;
    ULONG SplitCompletionMask : 8;
    ULONG HubAddr             : 7;
    ULONG PortNumber          : 7;
    ULONG PipeMultiplier      : 2;
  };
  ULONG AsULONG;
} EHCI_QH_EP_CAPS;

typedef struct _EHCI_QUEUE_HEAD { // must be aligned on 32-byte boundaries
  EHCI_LINK_POINTER HorizontalLink;
  EHCI_QH_EP_PARAMS EndpointParams;
  EHCI_QH_EP_CAPS EndpointCaps;
  ULONG_PTR CurrentTD;
  ULONG_PTR NextTD;
  ULONG_PTR AlternateNextTD;
  EHCI_TD_TOKEN Token;
  ULONG_PTR Buffer[5];
  ULONG_PTR ExtendedBuffer[5];
} EHCI_QUEUE_HEAD, *PEHCI_QUEUE_HEAD;

C_ASSERT(sizeof(EHCI_QUEUE_HEAD) == 68);
