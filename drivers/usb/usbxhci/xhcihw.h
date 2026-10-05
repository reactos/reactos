/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     xHCI hardware definitions (registers, TRBs, contexts)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Layouts follow xHCI 1.2; each block cites its spec section */

/* TRBs (xHCI 6.4) ************************************************************/

/** Every TRB is 16 bytes. Dword 3 bit 0 is the cycle bit, bits 15:10 the type. */
typedef struct _XHCI_TRB
{
    ULONG Dword[4];
} XHCI_TRB, *PXHCI_TRB;

C_ASSERT(sizeof(XHCI_TRB) == 16);

#define XHCI_TRB_CYCLE                  0x00000001
#define XHCI_TRB_TYPE_SHIFT             10
#define XHCI_TRB_TYPE_MASK              0x0000FC00
#define XHCI_TRB_SLOT_SHIFT             24
#define XHCI_TRB_ENDPOINT_SHIFT         16
#define XHCI_TRB_ENDPOINT_MASK          0x001F0000
#define XHCI_TRB_CODE_SHIFT             24

FORCEINLINE
ULONG
XhciTrbType(
    _In_ const XHCI_TRB* Trb)
{
    return (Trb->Dword[3] & XHCI_TRB_TYPE_MASK) >> XHCI_TRB_TYPE_SHIFT;
}

FORCEINLINE
ULONG
XhciTrbCompletionCode(
    _In_ const XHCI_TRB* Trb)
{
    return Trb->Dword[2] >> XHCI_TRB_CODE_SHIFT;
}

FORCEINLINE
ULONG
XhciTrbSlotId(
    _In_ const XHCI_TRB* Trb)
{
    return Trb->Dword[3] >> XHCI_TRB_SLOT_SHIFT;
}

FORCEINLINE
ULONG64
XhciTrbPointer(
    _In_ const XHCI_TRB* Trb)
{
    return ((ULONG64)Trb->Dword[1] << 32) | Trb->Dword[0];
}

/* TRB type codes (xHCI 6.4.6) */
enum class XhciTrbType : ULONG
{
    Normal = 1,
    SetupStage = 2,
    DataStage = 3,
    StatusStage = 4,
    Isoch = 5,
    Link = 6,
    EventData = 7,
    NoOp = 8,
    EnableSlot = 9,
    DisableSlot = 10,
    AddressDevice = 11,
    ConfigureEndpoint = 12,
    EvaluateContext = 13,
    ResetEndpoint = 14,
    StopEndpoint = 15,
    SetTrDequeuePointer = 16,
    ResetDevice = 17,
    ForceEvent = 18,
    NegotiateBandwidth = 19,
    SetLatencyToleranceValue = 20,
    GetPortBandwidth = 21,
    ForceHeader = 22,
    NoOpCommand = 23,
    TransferEvent = 32,
    CommandCompletionEvent = 33,
    PortStatusChangeEvent = 34,
    BandwidthRequestEvent = 35,
    DoorbellEvent = 36,
    HostControllerEvent = 37,
    DeviceNotificationEvent = 38,
    MfindexWrapEvent = 39,

    /* Vendor defined, used for firmware version queries */
    VendorCommandCompletionEvent = 48,
    VendorFirmwareVersion = 49,
    VendorFirmwareVersionAlt = 51,
    VendorFirmwareVersionAltSetup = 52
};

/* Completion codes (xHCI 6.4.5) */
enum class XhciCompletionCode : ULONG
{
    Invalid = 0,
    Success = 1,
    DataBufferError = 2,
    BabbleDetected = 3,
    UsbTransactionError = 4,
    TrbError = 5,
    StallError = 6,
    ResourceError = 7,
    BandwidthError = 8,
    NoSlotsAvailable = 9,
    InvalidStreamType = 10,
    SlotNotEnabled = 11,
    EndpointNotEnabled = 12,
    ShortPacket = 13,
    RingUnderrun = 14,
    RingOverrun = 15,
    VfEventRingFull = 16,
    ParameterError = 17,
    BandwidthOverrun = 18,
    ContextStateError = 19,
    NoPingResponse = 20,
    EventRingFull = 21,
    IncompatibleDevice = 22,
    MissedService = 23,
    CommandRingStopped = 24,
    CommandAborted = 25,
    Stopped = 26,
    StoppedLengthInvalid = 27,
    StoppedShortPacket = 28,
    MaxExitLatencyTooLarge = 29,
    IsochBufferOverrun = 31,
    EventLost = 32,
    UndefinedError = 33,
    InvalidStreamId = 34,
    SecondaryBandwidthError = 35,
    SplitTransactionError = 36,
    VendorQuirk199 = 199
};

/* Interrupter register set (xHCI 5.5.2), runtime base + 0x20 + 32 * n */
#define XHCI_RUNTIME_INTERRUPTER_BASE   0x20
#define XHCI_INTERRUPTER_STRIDE         0x20
#define XHCI_IMAN                       0x00
#define XHCI_IMOD                       0x04
#define XHCI_ERSTSZ                     0x08
#define XHCI_ERSTBA                     0x10
#define XHCI_ERDP                       0x18

#define XHCI_IMAN_PENDING               0x00000001
#define XHCI_IMAN_ENABLE                0x00000002
#define XHCI_ERDP_BUSY                  0x00000008

/** Event ring segment table entry (xHCI 6.5) */
typedef struct _XHCI_ERST_ENTRY
{
    ULONG64 SegmentAddress;
    ULONG SegmentSize;
    ULONG Reserved;
} XHCI_ERST_ENTRY, *PXHCI_ERST_ENTRY;

C_ASSERT(sizeof(XHCI_ERST_ENTRY) == 16);


/* command */

/* Command Ring Control Register bits (xHCI 5.4.5) */
#define XHCI_CRCR_RING_CYCLE            0x00000001
#define XHCI_CRCR_STOP                  0x00000002
#define XHCI_CRCR_ABORT                 0x00000004
#define XHCI_CRCR_RUNNING               0x00000008
#define XHCI_CRCR_POINTER_MASK          0xFFFFFFFFFFFFFFC0ULL

/* Link TRB dword 3 Toggle Cycle (xHCI 6.4.4.1) */
#define XHCI_LINK_TOGGLE_CYCLE          0x00000002

/* buffers */

/* Operational registers owned by the slot table (xHCI 5.4.6, 5.4.7) */
#define XHCI_OP_DCBAAP                  0x30
#define XHCI_OP_CONFIG                  0x38
#define XHCI_CONFIG_MAX_SLOTS_MASK      0x000000FF

/* DCBAA: entry 0 is the scratchpad array, 1..255 the output device contexts (xHCI 6.1) */
#define XHCI_DCBAA_ENTRIES              256

/* Device Notification Event dword 0 (xHCI 6.4.2.7) */
#define XHCI_NOTIFICATION_TYPE_SHIFT    4
#define XHCI_NOTIFICATION_TYPE_MASK     0x000000F0

/* roothub */

/* Port register set offsets for ReadPort32/WritePort32 (xHCI 5.4.8 to 5.4.11) */
#define XHCI_PORT_SC                    0x00
#define XHCI_PORT_PMSC                  0x04
#define XHCI_PORT_LI                    0x08
#define XHCI_PORT_HLPMC                 0x0C

/* PORTSC (xHCI 5.4.8) */
#define XHCI_PORTSC_CCS                 0x00000001
#define XHCI_PORTSC_PED                 0x00000002
#define XHCI_PORTSC_OCA                 0x00000008
#define XHCI_PORTSC_PR                  0x00000010
#define XHCI_PORTSC_PLS_SHIFT           5
#define XHCI_PORTSC_PLS_MASK            0x000001E0
#define XHCI_PORTSC_PP                  0x00000200
#define XHCI_PORTSC_SPEED_SHIFT         10
#define XHCI_PORTSC_SPEED_MASK          0x00003C00
#define XHCI_PORTSC_PIC_SHIFT           14
#define XHCI_PORTSC_PIC_MASK            0x0000C000
#define XHCI_PORTSC_LWS                 0x00010000
#define XHCI_PORTSC_CSC                 0x00020000
#define XHCI_PORTSC_PEC                 0x00040000
#define XHCI_PORTSC_WRC                 0x00080000
#define XHCI_PORTSC_OCC                 0x00100000
#define XHCI_PORTSC_PRC                 0x00200000
#define XHCI_PORTSC_PLC                 0x00400000
#define XHCI_PORTSC_CEC                 0x00800000
#define XHCI_PORTSC_CAS                 0x01000000
#define XHCI_PORTSC_WAKE_SHIFT          25
#define XHCI_PORTSC_WAKE_MASK           0x0E000000
#define XHCI_PORTSC_DR                  0x40000000
#define XHCI_PORTSC_WPR                 0x80000000

/** RWS bits carried over by every PORTSC write: PP, PIC and the wake enables. */
#define XHCI_PORTSC_PRESERVE            (XHCI_PORTSC_PP | XHCI_PORTSC_PIC_MASK | XHCI_PORTSC_WAKE_MASK)

/* PLS values beyond the hub link states in usb200.h */
#define XHCI_PLS_RESUME                 15

/* Port speed IDs in PORTSC (xHCI 7.2.2.1.1 default table) */
#define XHCI_SPEED_FULL                 1
#define XHCI_SPEED_LOW                  2
#define XHCI_SPEED_HIGH                 3
#define XHCI_SPEED_SUPER                4

/* PORTPMSC, USB 3.x layout (xHCI 5.4.9.1) */
#define XHCI_PORTPMSC_U1_TIMEOUT_MASK   0x000000FF
#define XHCI_PORTPMSC_U2_TIMEOUT_SHIFT  8
#define XHCI_PORTPMSC_U2_TIMEOUT_MASK   0x0000FF00
#define XHCI_PORTPMSC_FLA               0x00010000

/* PORTPMSC, USB 2.x layout (xHCI 5.4.9.2) */
#define XHCI_PORTPMSC_RWE               0x00000008
#define XHCI_PORTPMSC_BESL_SHIFT        4
#define XHCI_PORTPMSC_BESL_MASK         0x000000F0
#define XHCI_PORTPMSC_L1_SLOT_SHIFT     8
#define XHCI_PORTPMSC_L1_SLOT_MASK      0x0000FF00
#define XHCI_PORTPMSC_HLE               0x00010000
#define XHCI_PORTPMSC_TEST_SHIFT        28
#define XHCI_PORTPMSC_TEST_MASK         0xF0000000

/* PORTLI (xHCI 5.4.10) */
#define XHCI_PORTLI_ERROR_COUNT_MASK    0x0000FFFF
#define XHCI_PORTLI_RX_LANES_SHIFT      16
#define XHCI_PORTLI_RX_LANES_MASK       0x000F0000
#define XHCI_PORTLI_TX_LANES_SHIFT      20
#define XHCI_PORTLI_TX_LANES_MASK       0x00F00000

/* PORTHLPMC, USB 2.x (xHCI 5.4.11.2) */
#define XHCI_PORTHLPMC_HIRDM_MASK       0x00000003
#define XHCI_PORTHLPMC_L1_TIMEOUT_SHIFT 2
#define XHCI_PORTHLPMC_L1_TIMEOUT_MASK  0x000003FC
#define XHCI_PORTHLPMC_BESLD_SHIFT      10
#define XHCI_PORTHLPMC_BESLD_MASK       0x00003C00

/* Supported Protocol Capability (xHCI 7.2), byte offsets from the capability */
#define XHCI_EXTCAP_SUPPORTED_PROTOCOL  2
#define XHCI_PROTOCOL_REVISION          0x00
#define XHCI_PROTOCOL_NAME              0x04
#define XHCI_PROTOCOL_PORTS             0x08
#define XHCI_PROTOCOL_PSI               0x10

#define XHCI_PROTOCOL_MINOR_SHIFT       16
#define XHCI_PROTOCOL_MAJOR_SHIFT       24
#define XHCI_PROTOCOL_NAME_USB          0x20425355

#define XHCI_PROTOCOL_OFFSET_MASK       0x000000FF
#define XHCI_PROTOCOL_COUNT_SHIFT       8
#define XHCI_PROTOCOL_COUNT_MASK        0x0000FF00
#define XHCI_PROTOCOL_HSO               0x00020000
#define XHCI_PROTOCOL_IHI               0x00040000
#define XHCI_PROTOCOL_HLC               0x00080000
#define XHCI_PROTOCOL_BLC               0x00100000
#define XHCI_PROTOCOL_MHD_SHIFT         25
#define XHCI_PROTOCOL_MHD_MASK          0x0E000000
#define XHCI_PROTOCOL_PSIC_SHIFT        28

/* Protocol Speed ID dword (xHCI 7.2.1) */
#define XHCI_PSI_ID_MASK                0x0000000F
#define XHCI_PSI_EXPONENT_SHIFT         4
#define XHCI_PSI_EXPONENT_MASK          0x00000030
#define XHCI_PSI_TYPE_SHIFT             6
#define XHCI_PSI_TYPE_MASK              0x000000C0
#define XHCI_PSI_MANTISSA_SHIFT         16

#define XHCI_PSI_TYPE_SYMMETRIC         0
#define XHCI_PSI_TYPE_ASYMMETRIC_RX     2
#define XHCI_PSI_TYPE_ASYMMETRIC_TX     3

/* Debug Capability (xHCI 7.6.8): DCST holds the debug port number in bits 31:24 */
#define XHCI_EXTCAP_DEBUG               10
#define XHCI_DBC_DCST                   0x24
#define XHCI_DBC_DCST_PORT_SHIFT        24

/* Port Status Change Event: Port ID in dword 0 bits 31:24 (xHCI 6.4.2.3) */
#define XHCI_EVENT_PORT_ID_SHIFT        24

/* interrupter */

/* Transfer Event dword 3 Event Data flag (xHCI 6.4.2.1) */
#define XHCI_TRANSFER_EVENT_ED          0x00000004

/* Bits 1:0 of a driver written Event Data value carry the USB endpoint type */
#define XHCI_EVENT_DATA_TYPE_MASK       0x00000003

/* ERDP Dequeue ERST Segment Index (xHCI 5.5.2.3.3) */
#define XHCI_ERDP_SEGMENT_MASK          0x00000007

/* IMOD value for every enabled interrupter: 200 x 250 ns, counter 0 */
#define XHCI_IMOD_INTERVAL_50US         0x000000C8

/* controller */

/* Capability registers, offsets from the BAR (xHCI 5.3) */
#define XHCI_CAP_LENGTH_VERSION         0x00
#define XHCI_CAP_HCSPARAMS1             0x04
#define XHCI_CAP_HCSPARAMS2             0x08
#define XHCI_CAP_HCSPARAMS3             0x0C
#define XHCI_CAP_HCCPARAMS1             0x10
#define XHCI_CAP_DBOFF                  0x14
#define XHCI_CAP_RTSOFF                 0x18
#define XHCI_CAP_HCCPARAMS2             0x1C

#define XHCI_CAPLENGTH_MASK             0x000000FF
#define XHCI_HCIVERSION_MAJOR_SHIFT     24
#define XHCI_HCIVERSION_MINOR_SHIFT     16

/* HCSPARAMS1 (xHCI 5.3.3) */
#define XHCI_HCS1_MAX_SLOTS_MASK        0x000000FF
#define XHCI_HCS1_MAX_INTRS_SHIFT       8
#define XHCI_HCS1_MAX_INTRS_MASK        0x0007FF00
#define XHCI_HCS1_MAX_PORTS_SHIFT       24

/* HCSPARAMS2 (xHCI 5.3.4) */
#define XHCI_HCS2_IST_MASK              0x0000000F
#define XHCI_HCS2_ERST_MAX_SHIFT        4
#define XHCI_HCS2_ERST_MAX_MASK         0x000000F0
#define XHCI_HCS2_SCRATCH_HI_SHIFT      21
#define XHCI_HCS2_SCRATCH_HI_MASK       0x03E00000
#define XHCI_HCS2_SPR                   0x04000000
#define XHCI_HCS2_SCRATCH_LO_SHIFT      27

/* HCSPARAMS3 (xHCI 5.3.5) */
#define XHCI_HCS3_U1_LATENCY_MASK       0x000000FF
#define XHCI_HCS3_U2_LATENCY_SHIFT      16

/* HCCPARAMS1 (xHCI 5.3.6) */
#define XHCI_HCC1_AC64                  0x00000001
#define XHCI_HCC1_CSZ                   0x00000004
#define XHCI_HCC1_CFC                   0x00000800
#define XHCI_HCC1_MAX_PSA_SHIFT         12
#define XHCI_HCC1_MAX_PSA_MASK          0x0000F000
#define XHCI_HCC1_XECP_SHIFT            16

/* HCCPARAMS2 (xHCI 5.3.9) */
#define XHCI_HCC2_CMC                   0x00000002
#define XHCI_HCC2_LEC                   0x00000010
#define XHCI_HCC2_ETC                   0x00000040

/* Operational registers, offsets from the operational base (xHCI 5.4) */
#define XHCI_OP_USBCMD                  0x00
#define XHCI_OP_USBSTS                  0x04
#define XHCI_OP_PAGESIZE                0x08
#define XHCI_OP_DNCTRL                  0x14
#define XHCI_OP_CRCR                    0x18
#define XHCI_OP_PORT_BASE               0x400
#define XHCI_OP_PORT_STRIDE             0x10

/* USBCMD (xHCI 5.4.1) */
#define XHCI_USBCMD_RS                  0x00000001
#define XHCI_USBCMD_HCRST               0x00000002
#define XHCI_USBCMD_INTE                0x00000004
#define XHCI_USBCMD_HSEE                0x00000008
#define XHCI_USBCMD_CSS                 0x00000100
#define XHCI_USBCMD_CRS                 0x00000200
#define XHCI_USBCMD_EWE                 0x00000400
#define XHCI_USBCMD_CME                 0x00002000
#define XHCI_USBCMD_ETE                 0x00004000

/* USBSTS (xHCI 5.4.2) */
#define XHCI_USBSTS_HCH                 0x00000001
#define XHCI_USBSTS_HSE                 0x00000004
#define XHCI_USBSTS_EINT                0x00000008
#define XHCI_USBSTS_PCD                 0x00000010
#define XHCI_USBSTS_SSS                 0x00000100
#define XHCI_USBSTS_RSS                 0x00000200
#define XHCI_USBSTS_SRE                 0x00000400
#define XHCI_USBSTS_CNR                 0x00000800
#define XHCI_USBSTS_HCE                 0x00001000

/* PAGESIZE bit 0 means 4 KB pages (xHCI 5.4.3) */
#define XHCI_PAGESIZE_4K                0x00000001

/* DNCTRL N1, Function Wake device notifications (xHCI 5.4.4) */
#define XHCI_DNCTRL_FUNCTION_WAKE       0x00000002

/* Runtime registers (xHCI 5.5) */
#define XHCI_RT_MFINDEX                 0x00
#define XHCI_MFINDEX_MASK               0x00003FFF

/* Doorbell array (xHCI 5.6), one dword per slot, 0 is the command ring */
#define XHCI_DOORBELL_STRIDE            4

/* Extended capability header (xHCI 7) */
#define XHCI_EXTCAP_ID_MASK             0x000000FF
#define XHCI_EXTCAP_NEXT_SHIFT          8
#define XHCI_EXTCAP_NEXT_MASK           0x0000FF00
#define XHCI_EXTCAP_LEGACY              1

/* USB Legacy Support Capability (xHCI 7.1), byte offsets from the capability */
#define XHCI_LEGSUP_BIOS_OWNED_BYTE     0x02
#define XHCI_LEGSUP_OS_OWNED_BYTE       0x03
#define XHCI_LEGSUP_OWNED               0x01
#define XHCI_LEGSUP_CTLSTS              0x04

/** USBLEGCTLSTS write mask: SMI on OS Ownership off, the RW1C bits 31:29 written as 0. */
#define XHCI_LEGCTLSTS_KEEP_MASK        0x1FFFDFFF
