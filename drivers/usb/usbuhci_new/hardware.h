/* UHCI hardware registers */

#define UHCI_USBCMD     0x00 // USB Command
#define UHCI_USBSTS     0x02 // USB Status
#define UHCI_USBINTR    0x04 // USB Interrupt Enable
#define UHCI_FRNUM      0x06 // Frame Number
#define UHCI_FRBASEADD  0x08 // Frame List Base Address
#define UHCI_SOFMOD     0x0C // Start Of Frame Modify
#define UHCI_PORTSC1    0x10 // Port 1 Status/Control
#define UHCI_PORTSC2    0x12 // Port 2 Status/Control

/* Command register */

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

/* Status register */

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

/* Interrupt enable register */

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

/* USB port status and control registers */

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

/* Transfer Descriptor (TD) */

#define UHCI_TD_STS_ACTIVE            (1 << 7) // 0x80
#define UHCI_TD_STS_STALLED           (1 << 6) // 0x40
#define UHCI_TD_STS_DATA_BUFFER_ERROR (1 << 5) // 0x20
#define UHCI_TD_STS_BABBLE_DETECTED   (1 << 4) // 0x10
#define UHCI_TD_STS_NAK_RECEIVED      (1 << 3) // 0x08
#define UHCI_TD_STS_TIMEOUT_CRC_ERROR (1 << 2) // 0x04
#define UHCI_TD_STS_BITSTUFF_ERROR    (1 << 1) // 0x02
//#define UHCI_TD_STS_Reserved        (1 << 0) // 0x01

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

typedef struct _UHCI_TD { // always aligned on 16-byte boundaries
  ULONG_PTR NextElement; // another TD, or a QH, or nothing
  UHCI_CONTROL_STATUS ControlStatus;
  UHCI_TD_TOKEN Token;
  ULONG_PTR Buffer;
} UHCI_TD, *PUHCI_TD;

C_ASSERT(sizeof(UHCI_TD) == 16);
