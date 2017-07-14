#define UHCI_FRAME_LIST_MAX_ENTRIES  1024 // Number of frames in Frame List
#define UHCI_NUM_ROOT_HUB_PORTS      2

/* UHCI HC I/O Registers offset (PUSHORT) */
#define UHCI_USBCMD     0  // USB Command R/W
#define UHCI_USBSTS     1  // USB Status R/WC
#define UHCI_USBINTR    2  // USB Interrupt Enable R/W
#define UHCI_FRNUM      3  // Frame Number R/W WORD writeable only
#define UHCI_FRBASEADD  4  // Frame List Base Address  R/W // 32 bit
#define UHCI_SOFMOD     6  // Start Of Frame Modify  R/W // 8 bit
#define UHCI_PORTSC1    8  // Port 1 Status/Control R/WC WORD writeable only
#define UHCI_PORTSC2    9  // Port 2 Status/Control R/WC WORD writeable only

/* PCI Legacy Support */
#define PCI_LEGSUP             0xC0   // Legacy Support register offset. R/WC
#define PCI_LEGSUP_USBPIRQDEN  0x2000
#define PCI_LEGSUP_CLEAR_SMI   0x8F00

/* LEGSUP Legacy support register (PCI  Configuration - Function 2) */
typedef union _UHCI_PCI_LEGSUP {
  struct {
    USHORT Smi60Read           : 1; // (60REN) Trap/SMI On 60h Read Enable. R/W. 
    USHORT Smi60Write          : 1; // (60WEN) Trap/SMI On 60h Write Enable. R/W. 
    USHORT Smi64Read           : 1; // (64REN) Trap/SMI On 64h Read Enable. R/W. 
    USHORT Smi64Write          : 1; // (64WEN) Trap/SMI On 64h Write Enable. R/W. 
    USHORT SmiIrq              : 1; // (USBSMIEN) Trap/SMI ON IRQ Enable. R/W.
    USHORT A20Gate             : 1; // (A20PTEN) A20Gate Pass Through Enable. R/W. 
    USHORT PassThroughStatus   : 1; // (PSS) Pass Through Status. RO.
    USHORT SmiEndPassThrough   : 1; // (SMIEPTE) SMI At End Of Pass Through Enable. R/W.
    USHORT TrapBy60ReadStatus  : 1; // (TBY60R) Trap By 60h Read Status. R/WC.  
    USHORT TrapBy60WriteStatus : 1; // (TBY60W) Trap By 60h Write Status. R/WC.
    USHORT TrapBy64ReadStatus  : 1; // (TBY64R) Trap By 64h Read Status. R/WC. 
    USHORT TrapBy64WriteStatus : 1; // (TBY64W) Trap By 64h Write Status. R/WC.
    USHORT UsbIrqStatus        : 1; // (USBIRQS) USB IRQ Status. RO.
    USHORT UsbPIRQ             : 1; // (USBPIRQDEN) USB PIRQ Enable.  R/W.
    USHORT Reserved            : 1; // Reserved
    USHORT EndA20GateStatus    : 1; // (A20PTS) End OF A20GATE Pass Through Status. R/WC. 
  };
  USHORT AsUSHORT;
} UHCI_PCI_LEGSUP;

/* USBCMD Command register */
typedef union _UHCI_USB_COMMAND {
  struct {
    USHORT Run           : 1;
    USHORT HcReset       : 1;
    USHORT GlobalReset   : 1;
    USHORT GlobalSuspend : 1;
    USHORT GlobalResume  : 1; // Force Global Resume
    USHORT SoftwareDebug : 1; // 0 - Normal Mode, 1 - Debug mode
    USHORT ConfigureFlag : 1; // no effect on the hardware
    USHORT MaxPacket     : 1; // 0 = 32, 1 = 64
    USHORT Reserved      : 8;
  };
  USHORT AsUSHORT;
} UHCI_USB_COMMAND;

/* USBSTS Status register */
#define UHCI_USB_STATUS_MASK  0x3F

typedef union _UHCI_USB_STATUS {
  struct {
    USHORT Interrupt       : 1; // due to IOC (Interrupt On Complete)
    USHORT ErrorInterrupt  : 1; // due to error
    USHORT ResumeDetect    : 1;
    USHORT HostSystemError : 1; // PCI problems
    USHORT HcProcessError  : 1; // Schedule is buggy
    USHORT HcHalted        : 1;
    USHORT Reserved        : 10;
  };
  USHORT AsUSHORT;
} UHCI_USB_STATUS;

/* USBINTR Interrupt enable register */
typedef union _UHCI_INTERRUPT_ENABLE {
  struct {
    USHORT TimeoutCRC          : 1; // Timeout/CRC error enable
    USHORT ResumeInterrupt     : 1;
    USHORT InterruptOnComplete : 1;
    USHORT ShortPacket         : 1;
    USHORT Reserved            : 12;
  };
  USHORT AsUSHORT;
} UHCI_INTERRUPT_ENABLE;

/* FRNUM Frame Number register */
#define UHCI_FRNUM_FRAME_MASK     0x7FF 
#define UHCI_FRNUM_INDEX_MASK     0x3FF 
#define UHCI_FRNUM_OVERFLOW_LIST  0x400 

/* PORTSC(1|2) USB port status and control registers */
typedef union _UHCI_PORT_STATUS_CONTROL {
  struct {
    USHORT CurrentConnectStatus    : 1;
    USHORT ConnectStatusChange     : 1;
    USHORT PortEnabledDisabled     : 1;
    USHORT PortEnableDisableChange : 1;
    USHORT LineStatus              : 2; // D+ and D-
    USHORT ResumeDetect            : 1;
    USHORT Reserved1               : 1; // always 1
    USHORT LowSpeedDevice          : 1; // LS device Attached
    USHORT PortReset               : 1;
    USHORT Reserved2               : 2; // Intel use it (not UHCI 1.1d spec)
    USHORT Suspend                 : 1;
    USHORT Reserved3               : 3; // write zeroes
  };
  USHORT AsUSHORT;
} UHCI_PORT_STATUS_CONTROL;

typedef struct _UHCI_HW_REGISTERS {
  UHCI_USB_COMMAND HcCommand; // R/W
  UHCI_USB_STATUS HcStatus; // R/WC
  UHCI_INTERRUPT_ENABLE HcInterruptEnable; // R/W
  USHORT FrameNumber; // R/W WORD writeable only
  ULONG FrameAddress; // R/W
  UCHAR SOF_Modify; // R/W
  UCHAR Reserved[3];
  UHCI_PORT_STATUS_CONTROL PortControl[UHCI_NUM_ROOT_HUB_PORTS]; // R/WC WORD writeable only
} UHCI_HW_REGISTERS, *PUHCI_HW_REGISTERS;

/* Transfer Descriptor (TD) */
#define UHCI_TD_STS_ACTIVE            (1 << 7)
#define UHCI_TD_STS_STALLED           (1 << 6)
#define UHCI_TD_STS_DATA_BUFFER_ERROR (1 << 5)
#define UHCI_TD_STS_BABBLE_DETECTED   (1 << 4)
#define UHCI_TD_STS_NAK_RECEIVED      (1 << 3)
#define UHCI_TD_STS_TIMEOUT_CRC_ERROR (1 << 2)
#define UHCI_TD_STS_BITSTUFF_ERROR    (1 << 1)
//#define UHCI_TD_STS_Reserved        (1 << 0)

typedef union _UHCI_CONTROL_STATUS {
  struct {
    ULONG ActualLength        : 11; // encoded as n - 1
    ULONG Reserved1           : 5;
    ULONG Status              : 8; // UHCI_TD_STS_ xxx
    ULONG InterruptOnComplete : 1;
    ULONG IsochronousType     : 1;
    ULONG LowSpeedDevice      : 1;
    ULONG ErrorCounter        : 2;
    ULONG ShortPacketDetect   : 1;
    ULONG Reserved2           : 2;
  };
  ULONG AsULONG;
} UHCI_CONTROL_STATUS;

#define UHCI_TD_PID_IN     0x69
#define UHCI_TD_PID_OUT    0xE1
#define UHCI_TD_PID_SETUP  0x2D

#define UHCI_TD_PID_DATA0  0
#define UHCI_TD_PID_DATA1  1

#define UHCI_TD_MAX_VALID_LENGTH    0x4FF
#define UHCI_TD_MAX_LENGTH_INVALID  0x7FE
#define UHCI_TD_MAX_LENGTH_NULL     0x7FF

typedef union _UHCI_TD_TOKEN {
  struct {
    ULONG PIDCode       : 8;
    ULONG DeviceAddress : 7;
    ULONG Endpoint      : 4;
    ULONG DataToggle    : 1;
    ULONG Reserved      : 1;
    ULONG MaximumLength : 11;
  };
  ULONG AsULONG;
} UHCI_TD_TOKEN;

#define UHCI_TD_LINK_PTR_VALID          (0 << 0)
#define UHCI_TD_LINK_PTR_TERMINATE      (1 << 0)
#define UHCI_TD_LINK_PTR_TD             (0 << 1)
#define UHCI_TD_LINK_PTR_QH             (1 << 1)
#define UHCI_TD_LINK_PTR_BREADTH_FIRST  (0 << 2)
#define UHCI_TD_LINK_PTR_DEPTH_FIRST    (1 << 2)

#define UHCI_TD_LINK_POINTER_MASK       0xFFFFFFF0

typedef struct _UHCI_TD {  // Transfer Descriptors always aligned on 16-byte boundaries
  //Hardware
  ULONG_PTR NextElement;
  UHCI_CONTROL_STATUS ControlStatus;
  UHCI_TD_TOKEN Token;
  ULONG_PTR Buffer;
} UHCI_TD, *PUHCI_TD;

C_ASSERT(sizeof(UHCI_TD) == 16);

/* Queue Header (QH) */
#define UHCI_QH_HEAD_LINK_PTR_VALID         (0 << 0)
#define UHCI_QH_HEAD_LINK_PTR_TERMINATE     (1 << 0)
#define UHCI_QH_HEAD_LINK_PTR_TD            (0 << 1)
#define UHCI_QH_HEAD_LINK_PTR_QH            (1 << 1)

#define UHCI_QH_ELEMENT_LINK_PTR_VALID      (0 << 0)
#define UHCI_QH_ELEMENT_LINK_PTR_TERMINATE  (1 << 0)
#define UHCI_QH_ELEMENT_LINK_PTR_TD         (0 << 1)
#define UHCI_QH_ELEMENT_LINK_PTR_QH         (1 << 1)

typedef struct _UHCI_QH { // Queue Heads must be aligned on a 16-byte boundary
  //Hardware
  ULONG_PTR NextQH;
  ULONG_PTR NextElement;
} UHCI_QH, *PUHCI_QH;

C_ASSERT(sizeof(UHCI_QH) == 8);
