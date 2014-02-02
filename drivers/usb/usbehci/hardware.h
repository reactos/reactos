#pragma once

//
// Host Controller Capability Registers
//
#define EHCI_CAPLENGTH                  0x00
#define EHCI_HCIVERSION                 0x02
#define EHCI_HCSPARAMS                  0x04
#define EHCI_HCCPARAMS                  0x08
#define EHCI_HCSP_PORTROUTE             0x0c


//
// Extended Capabilities
//
#define EHCI_ECP_SHIFT                  8
#define EHCI_ECP_MASK                   0xff
#define EHCI_LEGSUP_CAPID_MASK          0xff
#define EHCI_LEGSUP_CAPID               0x01
#define EHCI_LEGSUP_OSOWNED             (1 << 24)
#define EHCI_LEGSUP_BIOSOWNED           (1 << 16)


//
// EHCI Operational Registers
//
#define EHCI_USBCMD                     0x00
#define EHCI_USBSTS                     0x04
#define EHCI_USBINTR                    0x08
#define EHCI_FRINDEX                    0x0C
#define EHCI_CTRLDSSEGMENT              0x10
#define EHCI_PERIODICLISTBASE           0x14
#define EHCI_ASYNCLISTBASE              0x18
#define EHCI_CONFIGFLAG                 0x40
#define EHCI_PORTSC                     0x44

//
// Interrupt Register Flags
//
#define EHCI_USBINTR_INTE               0x01
#define EHCI_USBINTR_ERR                0x02
#define EHCI_USBINTR_PC                 0x04
#define EHCI_USBINTR_FLROVR             0x08
#define EHCI_USBINTR_HSERR              0x10
#define EHCI_USBINTR_ASYNC              0x20
// Bits 6:31 Reserved

//
// Status Register Flags
//
#define EHCI_STS_INT                    0x01
#define EHCI_STS_ERR                    0x02
#define EHCI_STS_PCD                    0x04
#define EHCI_STS_FLR                    0x08
#define EHCI_STS_FATAL                  0x10
#define EHCI_STS_IAA                    0x20
// Bits 11:6 Reserved
#define EHCI_STS_HALT                   0x1000
#define EHCI_STS_RECL                   0x2000
#define EHCI_STS_PSS                    0x4000
#define EHCI_STS_ASS                    0x8000
#define EHCI_ERROR_INT                  (EHCI_STS_FATAL | EHCI_STS_ERR)

//
// Port Register Flags
//
#define EHCI_PRT_CONNECTED              0x01
#define EHCI_PRT_CONNECTSTATUSCHANGE    0x02
#define EHCI_PRT_ENABLED                0x04
#define EHCI_PRT_ENABLEDSTATUSCHANGE    0x08
#define EHCI_PRT_OVERCURRENTACTIVE      0x10
#define EHCI_PRT_OVERCURRENTCHANGE      0x20
#define EHCI_PRT_FORCERESUME            0x40
#define EHCI_PRT_SUSPEND                0x80
#define EHCI_PRT_RESET                  0x100
#define EHCI_PRT_LINESTATUSA            0x400
#define EHCI_PRT_LINESTATUSB            0x800
#define EHCI_PRT_POWER                  0x1000
#define EHCI_PRT_RELEASEOWNERSHIP       0x2000

#define EHCI_PORTSC_DATAMASK    0xffffffd1

#define EHCI_IS_LOW_SPEED(x) (((x) & EHCI_PRT_LINESTATUSA) && !((x) & EHCI_PRT_LINESTATUSB))
//
// Terminate Pointer used for QueueHeads and Element Transfer Descriptors to mark Pointers as the end
//
#define TERMINATE_POINTER       0x01

//
// QUEUE ELEMENT TRANSFER DESCRIPTOR, defines and structs
//

//
// Token Flags
//
#define PID_CODE_OUT_TOKEN      0x00
#define PID_CODE_IN_TOKEN       0x01
#define PID_CODE_SETUP_TOKEN    0x02

#define DO_START_SPLIT          0x00
#define DO_COMPLETE_SPLIT       0x01

#define PING_STATE_DO_OUT       0x00
#define PING_STATE_DO_PING      0x01

typedef struct _PERIODICFRAMELIST
{
    PULONG VirtualAddr;
    PHYSICAL_ADDRESS PhysicalAddr;
    ULONG Size;
} PERIODICFRAMELIST, *PPERIODICFRAMELIST;

//
// QUEUE ELEMENT TRANSFER DESCRIPTOR TOKEN
//
typedef struct _QETD_TOKEN_BITS
{
    ULONG PingState:1;
    ULONG SplitTransactionState:1;
    ULONG MissedMicroFrame:1;
    ULONG TransactionError:1;
    ULONG BabbleDetected:1;
    ULONG DataBufferError:1;
    ULONG Halted:1;
    ULONG Active:1;
    ULONG PIDCode:2;
    ULONG ErrorCounter:2;
    ULONG CurrentPage:3;
    ULONG InterruptOnComplete:1;
    ULONG TotalBytesToTransfer:15;
    ULONG DataToggle:1;
} QETD_TOKEN_BITS, *PQETD_TOKEN_BITS;

//
// QUEUE ELEMENT TRANSFER DESCRIPTOR
//
typedef struct _QUEUE_TRANSFER_DESCRIPTOR
{
    //Hardware
    ULONG NextPointer;
    ULONG AlternateNextPointer;
    union
    {
        QETD_TOKEN_BITS Bits;
        ULONG DWord;
    } Token;
    ULONG BufferPointer[5];
    ULONG ExtendedBufferPointer[5];

    //Software
    ULONG PhysicalAddr;
    LIST_ENTRY DescriptorEntry;
    ULONG TotalBytesToTransfer;
} QUEUE_TRANSFER_DESCRIPTOR, *PQUEUE_TRANSFER_DESCRIPTOR;

C_ASSERT(FIELD_OFFSET(QUEUE_TRANSFER_DESCRIPTOR, PhysicalAddr) == 0x34);

//
// EndPointSpeeds Flags and END_POINT_CHARACTERISTICS
//
#define QH_ENDPOINT_FULLSPEED       0x00
#define QH_ENDPOINT_LOWSPEED        0x01
#define QH_ENDPOINT_HIGHSPEED       0x02
typedef struct _END_POINT_CHARACTERISTICS
{
    ULONG DeviceAddress:7;
    ULONG InactiveOnNextTransaction:1;
    ULONG EndPointNumber:4;
    ULONG EndPointSpeed:2;
    ULONG QEDTDataToggleControl:1;
    ULONG HeadOfReclamation:1;
    ULONG MaximumPacketLength:11;
    ULONG ControlEndPointFlag:1;
    ULONG NakCountReload:4;
} END_POINT_CHARACTERISTICS, *PEND_POINT_CHARACTERISTICS;

//
// Capabilities
//
typedef struct _END_POINT_CAPABILITIES
{
    ULONG InterruptScheduleMask:8;
    ULONG SplitCompletionMask:8;
    ULONG HubAddr:6;
    ULONG PortNumber:6;
    ULONG NumberOfTransactionPerFrame:2;
} END_POINT_CAPABILITIES, *PEND_POINT_CAPABILITIES;

//
// QUEUE HEAD Flags and Struct
//
#define QH_TYPE_IDT             0x00
#define QH_TYPE_QH              0x02
#define QH_TYPE_SITD            0x04
#define QH_TYPE_FSTN            0x06

typedef struct _QUEUE_HEAD
{
    //Hardware
    ULONG HorizontalLinkPointer;
    END_POINT_CHARACTERISTICS EndPointCharacteristics;
    END_POINT_CAPABILITIES EndPointCapabilities;
    // TERMINATE_POINTER not valid for this member
    ULONG CurrentLinkPointer;
    // TERMINATE_POINTER valid
    ULONG NextPointer;
    // TERMINATE_POINTER valid, bits 1:4 is NAK_COUNTERd
    ULONG AlternateNextPointer;
    // Only DataToggle, InterruptOnComplete, ErrorCounter, PingState valid
    union
    {
        QETD_TOKEN_BITS Bits;
        ULONG DWord;
    } Token;
    ULONG BufferPointer[5];
    ULONG ExtendedBufferPointer[5];

    //Software
    ULONG PhysicalAddr;
    LIST_ENTRY LinkedQueueHeads;
    LIST_ENTRY TransferDescriptorListHead;
    PVOID NextQueueHead;
    PVOID Request;
} QUEUE_HEAD, *PQUEUE_HEAD;

C_ASSERT(sizeof(END_POINT_CHARACTERISTICS) == 4);
C_ASSERT(sizeof(END_POINT_CAPABILITIES) == 4);

C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, HorizontalLinkPointer) == 0x00);
C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, EndPointCharacteristics) == 0x04);
C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, EndPointCapabilities) == 0x08);
C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, CurrentLinkPointer) == 0xC);
C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, NextPointer) == 0x10);
C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, AlternateNextPointer) == 0x14);
C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, Token) == 0x18);
C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, BufferPointer) == 0x1C);
C_ASSERT(FIELD_OFFSET(QUEUE_HEAD, PhysicalAddr) == 0x44);


//
// Command register content
//
typedef struct _EHCI_USBCMD_CONTENT
{
    ULONG Run : 1;
    ULONG HCReset : 1;
    ULONG FrameListSize : 2;
    ULONG PeriodicEnable : 1;
    ULONG AsyncEnable : 1;
    ULONG DoorBell : 1;
    ULONG LightReset : 1;
    ULONG AsyncParkCount : 2;
    ULONG Reserved : 1;
    ULONG AsyncParkEnable : 1;
    ULONG Reserved1 : 4;
    ULONG IntThreshold : 8;
    ULONG Reserved2 : 8;
} EHCI_USBCMD_CONTENT, *PEHCI_USBCMD_CONTENT;

typedef struct _EHCI_HCS_CONTENT
{
    ULONG PortCount : 4;
    ULONG PortPowerControl: 1;
    ULONG Reserved : 2;
    ULONG PortRouteRules : 1;
    ULONG PortPerCHC : 4;
    ULONG CHCCount : 4;
    ULONG PortIndicator : 1;
    ULONG Reserved2 : 3;
    ULONG DbgPortNum : 4;
    ULONG Reserved3 : 8;

} EHCI_HCS_CONTENT, *PEHCI_HCS_CONTENT;

typedef struct _EHCI_HCC_CONTENT
{
    ULONG CurAddrBits : 1;
    ULONG VarFrameList : 1;
    ULONG ParkMode : 1;
    ULONG Reserved : 1;
    ULONG IsoSchedThreshold : 4;
    ULONG EECPCapable : 8;
    ULONG Reserved2 : 16;

} EHCI_HCC_CONTENT, *PEHCI_HCC_CONTENT;

typedef struct _EHCI_CAPS {
    UCHAR Length;
    UCHAR Reserved;
    USHORT HCIVersion;
    union
    {
        EHCI_HCS_CONTENT HCSParams;
        ULONG HCSParamsLong;
    };
    union
    {
        EHCI_HCC_CONTENT HCCParams;
        ULONG HCCParamsLong;
    };
    UCHAR PortRoute [15];
} EHCI_CAPS, *PEHCI_CAPS;

typedef struct
{
    ULONG PortStatus;
    ULONG PortChange;
}EHCI_PORT_STATUS;

#define EHCI_INTERRUPT_ENTRIES_COUNT    (10 + 1)
#define EHCI_VFRAMELIST_ENTRIES_COUNT    128
#define EHCI_FRAMELIST_ENTRIES_COUNT     1024

#define MAX_AVAILABLE_BANDWIDTH 125 // Microseconds

#define EHCI_QH_CAPS_MULT_SHIFT 30			// Transactions per Micro-Frame
#define EHCI_QH_CAPS_MULT_MASK 0x03
#define EHCI_QH_CAPS_PORT_SHIFT 23			// Hub Port (Split-Transaction)
#define EHCI_QH_CAPS_PORT_MASK 0x7f
#define EHCI_QH_CAPS_HUB_SHIFT 16			// Hub Address (Split-Transaction)
#define EHCI_QH_CAPS_HUB_MASK  0x7f
#define EHCI_QH_CAPS_SCM_SHIFT  8			// Split Completion Mask
#define EHCI_QH_CAPS_SCM_MASK   0xff
#define EHCI_QH_CAPS_ISM_SHIFT  0			// Interrupt Schedule Mask
#define EHCI_QH_CAPS_ISM_MASK  0xff
