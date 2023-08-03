/*
 * PROJECT:     ReactOS USB XHCI Miniport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBXHCI hardware declarations
 * COPYRIGHT:   Copyright 2023 Ian Marco Moffett <ian@vegaa.systems>
 */

typedef union _XHCI_HC_STRUCTURAL_PARAMS1 {
    struct {
       ULONG MaxDeviceSlots  : 8;
       ULONG MaxInterrupters : 11;
       ULONG Rsvd            : 5;
       ULONG MaxPorts        : 8;
    };
    ULONG AsULONG;
} XHCI_HC_STRUCTURAL_PARAMS1;

typedef union _XHCI_HC_STRUCTURAL_PARAMS2 {
    struct {
        ULONG IsochronousSchedulingThreshold : 4;
        ULONG EventRingSegmentTableMax       : 4;
        ULONG Rsvd                           : 13;
        ULONG MaxScratchPadBuffersHi         : 5;    // High order 5 bits
        ULONG ScratchpadRestore              : 1;
        ULONG MaxScratchPadBuffersLo         : 5;    // Low order 5 bits
    };
    ULONG AsULONG;
} XHCI_HC_STRUCTURAL_PARAMS2;

typedef union _XHCI_HC_STRUCTURAL_PARAMS3 {
    struct {
        ULONG U1DeviceExitLatency    : 8;
        ULONG Rsvd                   : 8;
        USHORT U2DeviceExitLatency;
    };
    ULONG AsULONG;
} XHCI_HC_STRUCTURAL_PARAMS3;

/*
 * See section 5.3.6 (HCCPARAMS1), table 5-13 of
 * the xHCI spec for more information.
 */
typedef union _XHCI_HC_CAP_PARAMS1 {
    struct {
        ULONG AC64              : 1;   // 64-bit Addressing Capability
        ULONG BWNegotiationCap  : 1;
        ULONG ContextSize       : 1;
        ULONG PortPowerControl  : 1;
        ULONG PortIndicators    : 1;
        ULONG LightHCResetCap   : 1;
        ULONG LTC               : 1;  // Latency Tolerance Messaging Capability
        ULONG NSS               : 1;  // No Secondary SID Support
        ULONG ParseAllEventData : 1;
        ULONG SPC               : 1;  // Stopped - Short Packet Capability
        ULONG SEC               : 1;  // Stopped EDTLA Capability
        ULONG CFC               : 1;  // Contiguous Frame ID Capability
        ULONG MaxPSASize        : 4;  // Maximum Primary Stream Array Size
        USHORT ExtendedCapPtr;        // xECP
    };
    ULONG AsULONG;
} XHCI_HC_CAP_PARAMS1;

/*
 * See section 5.3.9 (HCCPARAMS2), table 5-16 of
 * the xHCI spec for more information.
 */
typedef union  _XHCI_HC_CAP_PARAMS2 {
    ULONG U3EntryCap        : 1;
    ULONG CMC               : 1;  // Configure Endpoint Command Max Exit Latency Too Large Capability
    ULONG FSC               : 1;  // Force Save Context Capability
    ULONG CTC               : 1;  // Compliance Transition Capability
    ULONG LEC               : 1;  // Large ESIT Payload Capability
    ULONG ConfigInfoCap     : 1;
    ULONG ExtendedTBCCap    : 1;
    ULONG ETC_TSC           : 1;  // Extended TBC TRB Status Capability
    ULONG GSC               : 1;  // Get/Set Extended Property Capability
    ULONG VTC               : 1;  // Virtualization Based Trusted I/O Capability
    ULONG Rsvd              : 22;
} XHCI_HC_CAP_PARAMS2;

typedef struct _XHCI_HC_CAPABILITY_REGISTERS {
    UCHAR CapLength;
    UCHAR Rsvd;
    USHORT InterfaceVersion;
    XHCI_HC_STRUCTURAL_PARAMS1 StructParams1;
    XHCI_HC_STRUCTURAL_PARAMS2 StructParams2;
    XHCI_HC_STRUCTURAL_PARAMS3 StructParams3;
    XHCI_HC_CAP_PARAMS1 CapParams1;
    DWORD DbOff;
    DWORD RtsOff;
    XHCI_HC_CAP_PARAMS2 CapParams2;
} XHCI_HC_CAPABILITY_REGISTERS, *PXHCI_HC_CAPABILITY_REGISTERS;

/*
 * See section 5.4.1 (USBCMD), table 5-20 of
 * the xHCI spec for more information.
 */
typedef union _XHCI_USB_COMMAND {
    struct {
        ULONG RunStop                   : 1;
        ULONG HcReset                   : 1;
        ULONG InterrupterEnable         : 1;
        ULONG HostSystemErrorEnable     : 1;
        ULONG Rsvd                      : 3;
        ULONG LightHcReset              : 1;
        ULONG ControllerSaveState       : 1;
        ULONG ControllerRestoreState    : 1;
        ULONG ControllerWrapEvent       : 1;
        ULONG EU3S                      : 1;    // Enable U3 MFINDEX Stop
        ULONG Rsvd1                     : 1;
        ULONG CemEnable                 : 1;
        ULONG ExtendedTbcEnable         : 1;
        ULONG TSC_EN                    : 1;   // Extended TBC TRB Status Enable
        ULONG VtioEnable                : 1;
        ULONG Rsvd2                     : 1;
    };
    ULONG AsULONG;
} XHCI_USB_COMMAND;

/*
 * See section 5.4.2 (USBSTS), table 5-21 of
 * the xHCI spec for more information.
 */
typedef union _XHCI_USB_STATUS {
    struct {
        ULONG HcHalted              : 1;
        ULONG Rsvd                  : 1;
        ULONG HostSystemError       : 1;
        ULONG EventInterrupt        : 1;
        ULONG PortChangeDetect      : 1;
        ULONG Rsvd1                 : 3;
        ULONG SaveStateStatus       : 1;
        ULONG RestoreStateStatus    : 1;
        ULONG SaveRestoreError      : 1;
        ULONG ControllerNotReady    : 1;
        ULONG HcError               : 1;
        ULONG Rsvd2                 : 19;
    };
    ULONG AsULONG;
} XHCI_USB_STATUS;

typedef union _XHCI_HC_CONFIG {
    struct {
        ULONG MaxSlotsEn    : 8;
        ULONG U3EntryEn     : 1;
        ULONG ConfigInfoEn  : 1;
        ULONG Rsvd          : 22;
    };
    ULONG AsULONG;
} XHCI_HC_CONFIG;

/*
 * Operational Registers
 */
typedef struct _XHCI_HC_OPER_REGS {
    XHCI_USB_COMMAND UsbCmd;
    XHCI_USB_STATUS UsbStatus;
    DWORD PageSize;
    DWORD Rsvd;
    DWORD Rsvd1;
    DWORD DevNotification;
    ULONG CmdRingControl;
    DWORD Rsvd2[4];
    ULONG DcbaaPtr;
    XHCI_HC_CONFIG Config;
    DWORD Rsvd3[241];
    DWORD PortStatusControl;
    DWORD PortPowerControl;
    DWORD PortLink;
    DWORD Rsvd4;
} XHCI_HC_OPER_REGS, *PXHCI_HC_OPER_REGS;

typedef struct _XHCI_COMMAND_TRB {
    ULONG BufPtr;
    DWORD Meta;
    DWORD Control;
} XHCI_COMMAND_TRB;
