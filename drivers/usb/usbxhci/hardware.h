#ifndef XHCI_HARWARE_H_
#define XHCI_HARWARE_H_

//
// XHCI Limits
//
#define XHCI_MAX_EVENTS   208
#define XHCI_MAX_COMMANDS  16
#define XHCI_MAX_TRBS      18
#define XHCI_MAX_TRANSFERS  8

//-------------------------------------------------------------------------------------
//
// Host Controller Capability Registers
//
#define XHCI_CAPLENGTH  0x00
//#define XHCI_Rsvd     0x01
#define XHCI_HCIVERSION 0x02
#define XHCI_HCSPARAMS1 0x04
#define XHCI_HCSPARAMS2 0x08
#define XHCI_HCSPARAMS3 0x0C
#define XHCI_HCCPARAMS1 0x10
#define XHCI_DBOFF      0x14
#define XHCI_RTSOFF     0x18
#define XHCI_HCCPARAMS2 0x1C

//
// Extended Capability
//
#define XHCI_LEGSUP_CAPID     0x00000001 // USB Legacy Support
#define XHCI_LEGSUP_OSOWNED   0x01000000 // OS Owned Semaphore
#define XHCI_LEGSUP_BIOSOWNED 0x00010000 // BIOS Owned Semaphore
#define XHCI_ECP_MASK         0x000000FF // ECP Mask
#define XHCI_NEXT_CAP_MASK    0x0000FF00 // Next xHCI Extended Capability Pointer
#define XHCI_NEXT_CAP_SHIFT   0x00000008

typedef struct _HCSPARAMS1
{
    ULONG MaxSlots :  8; // Number of Device Slots
    ULONG MaxIntrs : 11; // Number of Interrupters
    ULONG Rsvd     :  5; // Reserved
    ULONG MaxPorts :  8; // Number of Ports
}HCSPARAMS1, *PHCSPARAMS1;

typedef struct _HCSPARAMS2
{
    ULONG IST                 :  4; // Isochronous Scheduling Threshold
    ULONG ERSTMax             :  4; // Event Ring Segment Table Max
    ULONG Rsvd                : 13; // Reserved
    ULONG MaxScratchPadBufsHi :  5; // Max Scratchpad Buffers
    ULONG SPR                 :  1; // Scratchpad Restore
    ULONG MaxScratchPadBufsLo :  5; // Max Scratchpad Buffers
}HCSPARAMS2, *PHCSPARAMS2;

typedef struct _HCSPARAMS3
{
    ULONG U1DeviceExitLatency :  8; // U1 Device Exit Latency
    ULONG Rsvd                :  8; // Rezerved
    ULONG U2DeviceExitLatency : 16; // U1 Device Exit Latency
}HCSPARAMS3, *PHCSPARAMS3;

typedef struct _HCCPARAMS1
{
    ULONG AC64       :  1; // 64-bit Addressing Capability
    ULONG BNC        :  1; // BW Negotiation Capability
    ULONG CSZ        :  1; // Context Size
    ULONG PPC        :  1; // Port Power Control
    ULONG PIND       :  1; // Port Indicator
    ULONG LHRC       :  1; // Light HC Reset Capability
    ULONG LLTC       :  1; // Latency Tolerance Messaging Capability
    ULONG NSS        :  1; // No Secondary SID Support
    ULONG PAE        :  1; // Parse All Events Data
    ULONG SPC        :  1; // Stopped - Short Packet Capability
    ULONG SEC        :  1; // Stopped EDTLA Capability
    ULONG CFC        :  1; // Contigous Frame ID Capability
    ULONG MaxPSASize :  4; // Maximum Primary Stream Array Size
    ULONG xECP       : 16; // xHCI Extended Capabilities Pointer
}HCCPARAMS1, *PHCCPARAMS1;

typedef struct _HCCPARAMS2
{
    ULONG U3C      :  1; // U3 Entry Capability
    ULONG CMC      :  1; // Configure Endpoint Command Max Exit Latency Too Large Capability
    ULONG FSC      :  1; // Force Save Context Capability
    ULONG CTC      :  1; // Compilance Transition Capability
    ULONG LEC      :  1; // Large ESIT Payload Capability
    ULONG CIC      :  1; // Configuration Information Capability
    ULONG Reserved : 24; // Reserved
}HCCPARAMS2, *PHCCPARAMS2;


typedef struct _XHCI_CAPABILITY_REGS
{
    UCHAR CapLength;              // Capability Register Length     (Base offset: 0x00)
    UCHAR Rsvd;                   // Rezerved                       (Base offset: 0x01)
    USHORT HciVersion;            // Interface Version Number       (Base offset: 0x02)
    union
    {
        HCSPARAMS1 HcsParams1;
        ULONG      HcsParams1Long; // Structural Parameters 1       (Base offset: 0x04)
    };
    union
    {
        HCSPARAMS2 HcsParams2;
        ULONG      HcsParams2Long; // Structural Parameters 2       (Base offset: 0x08)
    };
    union
    {
        HCSPARAMS3 HcsParams3;
        ULONG      HcsParams3Long; // Structural Parameters 3       (Base offset: 0x0C)
    };
    union
    {
        HCCPARAMS1 HccParams1;
        ULONG      HccParams1Long; // Capability Parameters 1       (Base offset: 0x10)
    };
    ULONG DoorBellOffset;          // Doorbell offset               (Base offset: 0x14)
    ULONG RunTmRegSpaceOff;        // Runtime Register Space Offset (Base offset: 0x18)
    union
    {
        HCCPARAMS2 HccParams2;
        ULONG      HccParams2Long; // Capability Parameters 2       (Base offset: 0x1C)
    };
    UCHAR Reserved;                // CAPLENGTH-0x20                (Base offset: 0x20)
}XHCI_CAPABILITY_REGS, *PXHCI_CAPABILITY_REGS;

//-------------------------------------------------------------------------------------

//
// Host Controller Operational Registers
//
#define XHCI_USBCMD      0x00
#define XHCI_USBSTS      0x04
#define XHCI_PAGE_SIZE   0x08
// reserved 0xC-0x13
#define XHCI_DNCTRL      0x14
#define XHCI_CRCR_LOW    0x18
#define XHCI_CRCR_HIGH   0x1C
#define XHCI_DCBAAP_LOW  0x30
#define XHCI_DCBAAP_HIGH 0x34
#define XHCI_CONFIG      0x38

// USB Status Register
#define XHCI_STS_HCH  0x00000001
#define XHCI_STS_HSE  0x00000004
#define XHCI_STS_EINT 0x00000008
#define XHCI_STS_PCD  0x00000010
#define XHCI_STS_CNR  0x00000800
#define XHCI_STS_HCE  0x00001000

// USB Command Register
#define XHCI_CMD_RUN   0x01
#define XHCI_CMD_HCRST 0x02
#define XHCI_CMD_EIE   0x04
#define XHCI_CMD_HSEE  0x08

#define XHCI_CRCR_RCS  0x01 // Ring Cycle State

#define XHCI_GET_PAGE_SIZE(n) (1 << (12 + n))

typedef struct _USBCMD_REG
{
    ULONG RUNSTOP :  1; // Run/Stop
    ULONG HCRST   :  1; // Host Controller Reset
    ULONG INTE    :  1; // Interrupter Enable
    ULONG HSEE    :  1; // Host System Error Enable
    ULONG RsvdP1  :  3; // Reserved
    ULONG LHCRST  :  1; // Light Host Controller Reset
    ULONG CSS     :  1; // Controller Save State
    ULONG CRS     :  1; // Controller Restore State
    ULONG EWE     :  1; // Enable Warp Event
    ULONG EU3S    :  1; // Enable U3 MFINDEX Stop
    ULONG SPE     :  1; // Stopped - Short Packet Enable
    ULONG CEM     :  1; // CEM Enable
    ULONG RsvdP2  : 18; // Reserved
}USBCMD_REG, *PUSBCMD_REG;

typedef struct _USBSTS_REG
{
    ULONG HCH    :  1; // HCHalted
    ULONG RsvdZ1 :  1; // Reserved
    ULONG HSE    :  1; // Host System Error
    ULONG EINT   :  1; // Event Interrupt
    ULONG PCD    :  1; // Port Change Detect
    ULONG RsvdZ2 :  3; // Reserved
    ULONG SSS    :  1; // Save State Status
    ULONG RSS    :  1; // Restore State Status
    ULONG SRE    :  1; // Save/Restore Error
    ULONG CNR    :  1; // Controller Not Ready
    ULONG HCE    :  1; // Host Controller Error
    ULONG RsvdP  : 19; // Reserved
}USBSTS_REG, *PUSBSTS_REG;

typedef struct _CRCR_REG
{
    ULONG RCS   :  1; // Ring Cycle State
    ULONG CS    :  1; // Command Stop
    ULONG CA    :  1; // Command Abort
    ULONG CRR   :  1; // Command Ring Running
    ULONG RsvdP :  2; // Reserved
    ULONG CRP1  : 26; // Command RingPointer Part1
    ULONG CRP2  : 32; // Command RingPointer Part2
}CRCR_REG, *PCRCR_REG;

typedef struct _CONFIG_REG
{
    ULONG MaxSlotsEn :  8; // Max Device Slots Enabled
    ULONG U3E        :  1; // U3 Entry Enable
    ULONG CIE        :  1; // Configuration Inforation Enable
    ULONG RsvdP      : 22; // Reserved
}CONFIG_REG, *PCONFIG_REG;

//
// Port Status Registers (Section 5.4.8)
//
#define XHCI_PORTSC(n) (0x400 + (0x10 * (n)))

#define XHCI_PORT_CONNECT_STATUS          0x00000001
#define XHCI_PORT_ENABLED                 0x00000002
#define XHCI_PORT_RESERVED                0x00000004
#define XHCI_PORT_OVER_CURRENT_ACTIVE     0x00000008
#define XHCI_PORT_RESET                   0x00000010
#define XHCI_PORT_POWER                   0x00000200
#define XHCI_PORT_LINK_STATE_WRITE_STROBE 0x00010000
#define XHCI_PORT_CONNECT_STATUS_CHANGE   0x00020000
#define XHCI_PORT_ENABLED_CHANGE          0x00040000
#define XHCI_PORT_WARM_RESET_CHANGE       0x00080000
#define XHCI_PORT_OVER_CURRENT_CHANGE     0x00100000
#define XHCI_PORT_RESET_CHANGE            0x00200000
#define XHCI_PORT_LINK_STATE_CHANGE       0x00400000
#define XHCI_PORT_CONFIG_ERROR_CHANGE     0x00800000
#define XHCI_PORT_COLD_ATTACH_STATUS      0x01000000
#define XHCI_PORT_WAKE_CONNECT_ENABLE     0x02000000
#define XHCI_PORT_WAKE_DISCONNECT_ENABLE  0x04000000
#define XHCI_PORT_WARM_RESET              0x40000000

//
// port speed
//
#define XHCI_PORT_GET_SPEED(x)            (((x) >> 10) & 0x0F)
#define XHCI_PORT_FULL_SPEED              0x00000001
#define XHCI_PORT_LOW_SPEED               0x00000002
#define XHCI_PORT_HIGH_SPEED              0x00000003
#define XHCI_PORT_SUPER_SPEED             0x00000004

typedef struct _PORT_REGS_SET
{
    ULONG PORTSC;
    ULONG PORTPMSC;
    ULONG PORTLI;
    ULONG PORTHLPMC; // 16 bytes total
}PORT_REGS_SET, *PPORT_REGS_SET;

typedef struct _OPERATIONAL_REGS
{
    union
    {
        USBCMD_REG    USBCMD;
        ULONG         USBCMDLong;     // USB Command
    };
    union
    {
        USBSTS_REG    USBSTS;
        ULONG         USBSTSLong;    // USB Status
    };
    ULONG             PAGESIZE;      // Page size
    UCHAR             RsvdZ1[8];     // Reserved
    ULONG             DNCTRL;        // Device Notification Control
    union
    {
        CRCR_REG      CRCR;
        LARGE_INTEGER CRCRLargeInt;  // Command Ring Control
    };
    UCHAR             RsvdZ2[16];    // Reserved
    LARGE_INTEGER     DCBAAP;        // Device Context Base Address Array Pointer
    union
    {
        CONFIG_REG    CONFIG;
        ULONG         CONFIGLong;    // Configure (64 bytes till here)
    };
    UCHAR             RsvdZ3[964];   // Reserved (64 + 964 = 1024 bytes)
    PPORT_REGS_SET    PortInfo[255]; // Port info
}OPERATIONAL_REGS, *POPERATIONAL_REGS;

//-------------------------------------------------------------------------------------

//
// Host Controller Runtime Registers
//
#define XHCI_ERSTSZ_BASE    0x28
#define XHCI_ERSTBA_LOW     0x30
#define XHCI_ERSTBA_HIGH    0x34
#define XHCI_ERDP_BASE_LOW  0x38
#define XHCI_ERDP_BASE_HIGH 0x3C

#define XHCI_ERST_EHB       0x08

#define XHCI_IMAN_BASE      0x20
#define XHCI_IMAN_INTR_ENA  0x02 // Interrupt enable

typedef struct _MFINDEX
{
    ULONG MicroframeIndex : 14; // Microframe Index
    ULONG RsvdZ           : 18; // Reserved
}MFINDEX, *PMFINDEX;

typedef struct _IMAN
{
    ULONG IP    :  1; // Interrupt Pending
    ULONG IE    :  1; // Interrupt Enable
    ULONG RsvdP : 30; // Reserved
}IMAN, *PIMAN;

typedef struct _IMOD
{
    ULONG IMODI : 16; // Interrupt Moderation Interval
    ULONG IMODC : 16; // Interrupt Moderation Counter
}IMOD, *PIMOD;

typedef struct _ERSTSZ
{
    ULONG EventRingSegTabSize : 16; // Event Ring Segment Table Size
    ULONG RsvdP               : 16; // RsvdP
}ERSTSZ, *PERSTSZ;


typedef struct _RUNTIME_REGS
{
    union
    {
        IMAN      Iman;
        ULONG     ImanLong;   // Interrupter Management
    };

    union
    {
        IMOD      Imod;
        ULONG     ImodLong;   // Interrupter Moderation
    };

    union
    {
        ERSTSZ    Erstsz;
        ULONG     ErstszLong; // Event Ring Segment Table Size
    };

    ULONG         RsvdP;      // Reserved
    LARGE_INTEGER Erstba;     // Event Ring Segment Table Base Address
    LARGE_INTEGER Erdp;       // Event Ring Dequeue Pointer
}RUNTIME_REGS, *PRUNTIME_REGS;
//-------------------------------------------------------------------------------------

//
// Doorbell Registers
//
#define XHCI_DOORBELL(n)              (0x0000 + (4 * (n)))
#define XHCI_DOORBELL_TARGET(x)       ((x) & 0xFF)
#define XHCI_DOORBELL_TARGET_GET(x)   ((x) & 0xFF)
#define XHCI_DOORBELL_STREAMID(x)     (((x) & 0xFFFF) << 16)
#define XHCI_DOORBELL_STREAMID_GET(x) (((x) >> 16) & 0xFFFF)

typedef struct _DOORBELL_REGISTER
{
    ULONG DBTarget   : 8; // Doorbell Target
    ULONG RsvdZ      : 8; // Reserved
    ULONG DBStreamId : 8; // Doorbell Stream ID
}DOORBELL_REGISTER, *PDOORBELL_REGISTER;

typedef struct _DOORBELL_REG
{
    union
    {
        DOORBELL_REGISTER DoorBellReg;
        ULONG             DoorBellRegLong;
    };
}DOORBELL_REG, *PDOORBELL_REG;
//-------------------------------------------------------------------------------------

//
// 6.1 Device Context Base Address Array
//
typedef struct _DEVICE_CONTEXT_ARRAY
{
    PHYSICAL_ADDRESS  BaseAddress;
}DEVICE_CONTEXT_ARRAY, *PDEVICE_CONTEXT_ARRAY;

//
// 6.2.2 Slot Context
//
#define XHCI_GET_SLOT_ROUTE(x)            ((x) & 0xFFFFF)
#define XHCI_SLOT_ROUTE(x)                ((x) & 0xFFFFF)
#define XHCI_SLOT_SPEED(x)                (((x) & 0xF) << 20)
#define XHCI_GET_SLOT_SPEED(x)            (((x) >> 20) & 0xF)
#define XHCI_SLOT_MTT_BIT                 (1 << 25)
#define XHCI_SLOT_HUB_BIT                 (1 << 26)
#define XHCI_SLOT_NUM_ENTRIES(x)          (((x) & 0x1F) << 27)
#define XHCI_GET_SLOT_NUM_ENTRIES(x)      (((x) >> 27) & 0x1F)

#define XHCI_GET_SLOT_MAX_EXIT_LATENCY(x) ((x) & 0xFFFF)
#define XHCI_SLOT_MAX_EXIT_LATENCY(x)     ((x) & 0xFFFF)
#define XHCI_SLOT_RH_PORT(x)              (((x) & 0xFF) << 16)
#define XHCI_GET_SLOT_RH_PORT(x)          (((x) >> 16) & 0xFF)
#define XHCI_SLOT_NUM_PORTS(x)            (((x) & 0xFF) << 24)
#define XHCI_GET_SLOT_NUM_PORTS(x)        (((x) >> 24) & 0xFF)

#define XHCI_SLOT_TT_HUB_SLOT(x)          ((x) & 0xFF)
#define XHCI_GET_SLOT_TT_HUB_SLOT(x)      ((x) & 0xFF)
#define XHCI_SLOT_PORT_NUM(x)             (((x) & 0xFF) << 8)
#define XHCI_GET_SLOT_PORT_NUM(x)         (((x) >> 8) & 0xFF)
#define XHCI_SLOT_TT_TIME(x)              (((x) & 0x3) << 16)
#define XHCI_GET_SLOT_TT_TIME(x)          (((x) >> 16) & 0x3)
#define XHCI_SLOT_IRQ_TARGET(x)           (((x) & 0x7F) << 22)
#define XHCI_GET_SLOT_IRQ_TARGET(x)       (((x) >> 22) & 0x7F)

#define XHCI_SLOT_DEVICE_ADDRESS(x)       ((x) & 0xFF)
#define XHCI_GET_SLOT_DEVICE_ADDRESS(x)   ((x) & 0xFF)
#define XHCI_SLOT_SLOT_STATE(x)           (((x) & 0x1F) << 27)
#define XHCI_GET_SLOT_SLOT_STATE(x)       (((x) >> 27) & 0x1F)

#define XHCI_GET_HUB_TTT(x)               (((x) >> 5) & 0x3)

#define XHCI_DEVICE_SLOT_STATE_DISABLED   0x00
#define XHCI_DEVICE_SLOT_STATE_ENABLED    0x01
#define XHCI_DEVICE_SLOT_STATE_DEFAULT    0x02
#define XHCI_DEVICE_SLOT_STATE_ADDRESSED  0x03
#define XHCI_DEVICE_SLOT_STATE_CONFIGURED 0x04

typedef struct _SLOT_CONTEXT
{
    ULONG DeviceInfo;
    ULONG DeviceInfo2;
    ULONG TTInfo;
    ULONG DeviceState;
    ULONG Reserved[4];
}SLOT_CONTEXT, *PSLOT_CONTEXT;

//
// 6.2.3 Endpoint Context
//
#define XHCI_ENDPOINT_STATE(x)              ((x) & 0x3)
#define XHCI_GET_ENDPOINT_STATE_GET(x)      ((x) & 0x3)
#define XHCI_ENDPOINT_MULT(x)               (((x) & 0x3) << 8)
#define XHCI_GET_ENDPOINT_MULT(x)           (((x) >> 8) & 0x3)
#define XHCI_ENDPOINT_MAXPSTREAMS(x)        (((x) & 0x1F) << 10)
#define XHCI_GET_ENDPOINT_MAXPSTREAMS(x)    (((x) >> 10) & 0x1F)
#define XHCI_ENDPOINT_LSA_BIT               (1 << 15)
#define XHCI_ENDPOINT_INTERVAL(x)           (((x) & 0xFF) << 16)
#define XHCI_GET_ENDPOINT_INTERVAL(x)       (((x) >> 16) & 0xFF)

#define XHCI_ENDPOINT_CERR(x)               (((x) & 0x3) << 1)
#define XHCI_GET_ENDPOINT_CERR(x)           (((x) >> 1) & 0x3)
#define XHCI_ENDPOINT_EPTYPE(x)             (((x) & 0x7) << 3)
#define XHCI_GET_ENDPOINT_EPTYPE(x)         (((x) >> 3) & 0x7)
#define XHCI_ENDPOINT_HID_BIT               (1 << 7)
#define XHCI_ENDPOINT_MAXBURST(x)           (((x) & 0xFF) << 8)
#define XHCI_GET_ENDPOINT_MAXBURST(x)       (((x) >> 8) & 0xFF)
#define XHCI_ENDPOINT_MAXPACKETSIZE(x)      (((x) & 0xFFFF) << 16)
#define XHCI_GET_ENDPOINT_MAXPACKETSIZE(x)  (((x) >> 16) & 0xFFFF)

#define XHCI_ENDPOINT_DCS_BIT               (1 << 0)

#define XHCI_ENDPOINT_AVGTRBLENGTH(x)       ((x) & 0xFFFF)
#define XHCI_GET_ENDPOINT_AVGTRBLENGTH(x)   ((x) & 0xFFFF)
#define XHCI_ENDPOINT_MAXESITPAYLOAD(x)     (((x) & 0xFFFF) << 16)
#define XHCI_GET_ENDPOINT_MAXESITPAYLOAD(x) (((x) >> 16) & 0xFFFF)

#define XHCI_ENDPOINT_STATE_DISABLED        0x00
#define XHCI_ENDPOINT_STATE_RUNNING         0x01
#define XHCI_ENDPOINT_STATE_HALTED          0x02
#define XHCI_ENDPOINT_STATE_STOPPED         0x03
#define XHCI_ENDPOINT_STATE_ERROR           0x04

#define XHCI_ENDPOINT_ISOCHRONOUS_OUT       0x01
#define XHCI_ENDPOINT_BULK_OUT              0x02
#define XHCI_ENDPOINT_INTERRUPT_OUT         0x03
#define XHCI_ENDPOINT_CONTROL_BIDIRECTIONAL 0x04
#define XHCI_ENDPOINT_ISOCHRONOUS_IN        0x05
#define XHCI_ENDPOINT_BULK_IN               0x06
#define XHCI_ENDPOINT_INTERRUPT_IN          0x07

typedef struct _ENDPOINT_CONTEXT
{
    ULONG            EndpointInfo;
    ULONG            EndpointInfo2;
    PHYSICAL_ADDRESS Dequeue;
    ULONG            TxInfo;
    ULONG            Reserved[3];
}ENDPOINT_CONTEXT, *PENDPOINT_CONTEXT;

//
// 6.2.1 Device Context
//
typedef struct _DEVICE_CONTEXT
{
    SLOT_CONTEXT     SlotContext;
    ENDPOINT_CONTEXT Endpoints[31];
}DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// 6.2.4 Stream Context Array
//
typedef struct _STREAM_CONTEXT
{
    LARGE_INTEGER Stream0;
    ULONG         Reserved[2];
}STREAM_CONTEXT, *PSTREAM_CONTEXT;

//
// 6.2.5.1 Input Control Context
//
typedef struct _INPUT_CONTROL_CONTEXT
{
    ULONG DropFlags;
    ULONG AddFlags;
    ULONG Reserved[6];
}INPUT_CONTROL_CONTEXT, *PINPUT_CONTROL_CONTEXT;

//
// 6.2.5 Input Context
//
typedef struct _INPUT_DEVICE_CONTEXT
{
    INPUT_CONTROL_CONTEXT InputControl;
    SLOT_CONTEXT          SlotContext;
    ENDPOINT_CONTEXT      Endpoints[31];
}INPUT_DEVICE_CONTEXT, *PINPUT_DEVICE_CONTEXT;

//
// Macros for Event TRB
//
#define XHCI_TRB_TD_SIZE(x)             (((x) & 0x1F) << 17)
#define XHCI_TRB_GET_TD_SIZE(x)         (((x) >> 17) & 0x1F)
#define XHCI_TRB_GET_REMAINDER_BYTES(x) ((x) & 0xFFFFFF)
#define XHCI_TRB_GET_TRANSFER_LENGTH(x) ((x) & 0x1FFFF)
#define XHCI_TRB_IRQ(x)                 (((x) & 0x3FF) << 22)
#define XHCI_TRB_GET_IRQ(x)             (((x) >> 22) & 0x3FF)

#define XHCI_TRB_GET_TYPE(x)            (((x) >> 10) & 0x3F)
#define XHCI_TRB_TYPE(x)                (((x) & 0x3F) << 10)
#define XHCI_TRB_GET_SLOT(x)            (((x) >> 24) & 0xFF)
#define XHCI_TRB_SLOT(x)                (((x) & 0xFF) << 24)
#define XHCI_TRB_GET_ENDPOINT(x)        (((x) & 0x1F) >> 16)

#define XHCI_TRB_COMP_CODE(x)           (((x) >> 24) & 0xFF)

//
// TRB Type
//
#define XHCI_TRB_TYPE_NORMAL                  1
#define XHCI_TRB_TYPE_SETUP_STAGE             2
#define XHCI_TRB_TYPE_DATA_STAGE              3
#define XHCI_TRB_TYPE_STATUS_STAGE            4
#define XHCI_TRB_TYPE_ISOCH                   5
#define XHCI_TRB_TYPE_LINK                    6
#define XHCI_TRB_TYPE_EVENT_DATA              7
#define XHCI_TRB_TYPE_TR_NOOP                 8

//
// commands
//
#define XHCI_TRB_TYPE_ENABLE_SLOT             9
#define XHCI_TRB_TYPE_DISABLE_SLOT           10
#define XHCI_TRB_TYPE_ADDRESS_DEVICE         11
#define XHCI_TRB_TYPE_CONFIGURE_ENDPOINT     12
#define XHCI_TRB_TYPE_EVALUATE_CONTEXT       13
#define XHCI_TRB_TYPE_RESET_ENDPOINT         14
#define XHCI_TRB_TYPE_STOP_ENDPOINT          15
#define XHCI_TRB_TYPE_SET_TR_DEQUEUE         16
#define XHCI_TRB_TYPE_RESET_DEVICE           17
#define XHCI_TRB_TYPE_FORCE_EVENT            18
#define XHCI_TRB_TYPE_NEGOCIATE_BW           19
#define XHCI_TRB_TYPE_SET_LATENCY_TOLERANCE  20
#define XHCI_TRB_TYPE_GET_PORT_BW            21
#define XHCI_TRB_TYPE_FORCE_HEADER           22
#define XHCI_TRB_TYPE_CMD_NOOP               23

//
// events
//
#define XHCI_TRB_TYPE_TRANSFER               32
#define XHCI_TRB_TYPE_COMMAND_COMPLETION     33
#define XHCI_TRB_TYPE_PORT_STATUS_CHANGE     34
#define XHCI_TRB_TYPE_BANDWIDTH_REQUEST      35
#define XHCI_TRB_TYPE_DOORBELL               36
#define XHCI_TRB_TYPE_HOST_CONTROLLER        37
#define XHCI_TRB_TYPE_DEVICE_NOTIFICATION    38
#define XHCI_TRB_TYPE_MFINDEX_WRAP           39

//
// vendor
//
#define XHCI_TRB_TYPE_NEC_COMMAND_COMPLETION 48
#define XHCI_TRB_TYPE_NEC_GET_FIRMWARE_REV   49

//
// TRB completion Code
//
#define XHCI_TRB_COMPLETION_INVALID               0
#define XHCI_TRB_COMPLETION_SUCCESS               1
#define XHCI_TRB_COMPLETION_DATA_BUFFER           2
#define XHCI_TRB_COMPLETION_BABBLE                3
#define XHCI_TRB_COMPLETION_USB_TRANSACTION       4
#define XHCI_TRB_COMPLETION_TRB                   5
#define XHCI_TRB_COMPLETION_STALL                 6
#define XHCI_TRB_COMPLETION_RESOURCE              7
#define XHCI_TRB_COMPLETION_BANDWIDTH             8
#define XHCI_TRB_COMPLETION_NO_SLOTS              9
#define XHCI_TRB_COMPLETION_INVALID_STREAM       10
#define XHCI_TRB_COMPLETION_SLOT_NOT_ENABLED     11
#define XHCI_TRB_COMPLETION_ENDPOINT_NOT_ENABLED 12
#define XHCI_TRB_COMPLETION_SHORT_PACKET         13
#define XHCI_TRB_COMPLETION_RING_UNDERRUN        14
#define XHCI_TRB_COMPLETION_RING_OVERRUN         15
#define XHCI_TRB_COMPLETION_VF_RING_FULL         16
#define XHCI_TRB_COMPLETION_PARAMETER            17
#define XHCI_TRB_COMPLETION_BANDWIDTH_OVERRUN    18
#define XHCI_TRB_COMPLETION_CONTEXT_STATE        19
#define XHCI_TRB_COMPLETION_NO_PING_RESPONSE     20
#define XHCI_TRB_COMPLETION_EVENT_RING_FULL      21
#define XHCI_TRB_COMPLETION_INCOMPATIBLE_DEVICE  22
#define XHCI_TRB_COMPLETION_MISSED_SERVICE       23
#define XHCI_TRB_COMPLETION_COMMAND_RING_STOPPED 24
#define XHCI_TRB_COMPLETION_COMMAND_ABORTED      25
#define XHCI_TRB_COMPLETION_STOPPED              26
#define XHCI_TRB_COMPLETION_LENGTH_INVALID       27
#define XHCI_TRB_COMPLETION_MAX_EXIT_LATENCY     29
#define XHCI_TRB_COMPLETION_ISOC_OVERRUN         31
#define XHCI_TRB_COMPLETION_EVENT_LOST           32
#define XHCI_TRB_COMPLETION_UNDEFINED            33
#define XHCI_TRB_COMPLETION_INVALID_STREAM_ID    34
#define XHCI_TRB_COMPLETION_SECONDARY_BANDWIDTH  35
#define XHCI_TRB_COMPLETION_SPLIT_TRANSACTION    36

//
// TRB flags
//
#define XHCI_TRB_CYCLE_BIT            0x00000001
#define XHCI_TRB_TC_BIT               0x00000002
#define XHCI_TRB_ENT_BIT              0x00000002
#define XHCI_TRB_ISP_BIT              0x00000004
#define XHCI_TRB_NSNOOP_BIT           0x00000008
#define XHCI_TRB_CHAIN_BIT            0x00000010
#define XHCI_TRB_IOC_BIT              0x00000020
#define XHCI_TRB_IDT_BIT              0x00000040
#define XHCI_TRB_BEI_BIT              0x00000200
#define XHCI_TRB_DCEP_BIT             0x00000200
#define XHCI_TRB_PRSV_BIT             0x00000200
#define XHCI_TRB_BSR_BIT              0x00000200
#define XHCI_TRB_TRT_MASK             0x00030000
#define XHCI_TRB_DIR_IN               0x00010000
#define XHCI_TRB_TRT_OUT              0x00020000
#define XHCI_TRB_TRT_IN               0x00030000
#define XHCI_TRB_SUSPEND_ENDPOINT_BIT 0x00800000
#define XHCI_TRB_ISO_SIA_BIT          0x80000000

//
// 6.4 Transfer Request Block (TRB)
//
typedef struct _TRB
{
    ULONG Field[4];
}TRB, *PTRB;

//
// 6.5 Event Ring Segment Table
//
typedef struct DECLSPEC_ALIGN(64) _ERST_ELEMENT
{
    PHYSICAL_ADDRESS Address;
    ULONG            Size;
    ULONG            Reserved;
}ERST_ELEMENT, *PERST_ELEMENT;

//-------------------------------------------------------------------------------------

//
// command information
//
typedef struct _COMMAND_INFORMATION
{
    ULONG            CommandType;
    PHYSICAL_ADDRESS InputContext;
    BOOLEAN          BlockSetRequest;
    BOOLEAN          Deconfigure;
    ULONG            SlotId;
    ULONG            Endpoint;
    BOOLEAN          Preserve;
    ULONG            Stream;
}COMMAND_INFORMATION, *PCOMMAND_INFORMATION;

#define USB_TARGET_XHCI     1
#define USB_TARGET_DEVICE   2

//
// XHCI command descriptor
//
typedef struct _COMMAND_DESCRIPTOR
{
    // Hardware
    PTRB             PhysicalTrbAddress;

    // Software
    PTRB             VirtualTrbAddress;
    PTRB             CompletedTrb;
    ULONG            CommandType;
    PHYSICAL_ADDRESS DequeueAddress;
    PVOID            Request;
    LIST_ENTRY       DescriptorListEntry;
}COMMAND_DESCRIPTOR, *PCOMMAND_DESCRIPTOR;

//
// HCD endpoint state
//
typedef struct _ENDPOINT
{
    PHYSICAL_ADDRESS PhysicalRingbufferAddress;
    PVOID            VirtualRingbufferAddress;
    PTRB             EnqueuePointer, DequeuePointer;
    ULONG            CycleState;
    LIST_ENTRY       DescriptorListHead;
    ULONG            EndpointState;
}ENDPOINT, *PENDPOINT;

#define XHCI_SEARCH_DEVICE_BY_SLOT_ID   1
#define XHCI_SEARCH_DEVICE_BY_PORT_ID   2
#define XHCI_SEARCH_DEVICE_BY_ADDRESS   3
//
// device info
//
typedef struct _PDEVICE_INFORMATION
{
    ULONG                 SlotId;
    ULONG                 State;
    ULONG                 Address;

    PHYSICAL_ADDRESS      PhysicalInputContextAddress;
    PINPUT_DEVICE_CONTEXT InputContextAddress;

    PHYSICAL_ADDRESS      PhysicalDeviceContextAddress;
    PDEVICE_CONTEXT       DeviceContextAddress;

    ENDPOINT              Endpoints[31];

    ULONG                 PortId;
    LIST_ENTRY            DeviceListEntry;
}DEVICE_INFORMATION, *PDEVICE_INFORMATION;
#endif // XHCI_HARWARE_H_
