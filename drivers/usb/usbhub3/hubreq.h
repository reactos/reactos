/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Preallocated transfer requests and the control transfer sender
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Every control transfer the hub sends gives up after this long */
#define HUB_SETUP_REQUEST_TIMEOUT_MS 5000

/* No hub or port feature is selected for acknowledging */
#define HUB_FEATURE_NONE 1000

/** A WDFREQUEST and the URB it carries, reused for every control transfer of one owner. */
struct HubControlRequest
{
    WDFREQUEST Request;
    PIRP Irp;
    struct _URB_CONTROL_TRANSFER_EX Urb;

    /* Copied into UrbHeader.UsbdFlags on every send */
    ULONG UsbdFlags;

    _Must_inspect_result_
    NTSTATUS
    Create(
        _In_ WDFOBJECT Parent,
        _In_ WDFIOTARGET SizingTarget);

    /** Setup packet fields, little endian as on the wire. */
    VOID
    SetSetup(
        _In_ UCHAR RequestType,
        _In_ UCHAR RequestCode,
        _In_ USHORT Value,
        _In_ USHORT Index,
        _In_ USHORT Length)
    {
        Urb.SetupPacket[0] = RequestType;
        Urb.SetupPacket[1] = RequestCode;
        Urb.SetupPacket[2] = (UCHAR)(Value & 0xFF);
        Urb.SetupPacket[3] = (UCHAR)(Value >> 8);
        Urb.SetupPacket[4] = (UCHAR)(Index & 0xFF);
        Urb.SetupPacket[5] = (UCHAR)(Index >> 8);
        Urb.SetupPacket[6] = (UCHAR)(Length & 0xFF);
        Urb.SetupPacket[7] = (UCHAR)(Length >> 8);
    }

    /** On failure the request was refused and is ready for reuse; on success the completion owns it. */
    NTSTATUS
    Send(
        _In_ WDFIOTARGET Target,
        _In_opt_ UCXUSBDEVICE Device,
        _In_ PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion,
        _In_ WDFCONTEXT Context,
        _In_reads_bytes_opt_(Length) PVOID Buffer,
        _In_ ULONG Length,
        _In_ BOOLEAN ShortTransferOk,
        _In_ BOOLEAN ForwardProgress);

    VOID
    Reuse();
};

/** The status change interrupt transfer of a hub, with its change bitmap. */
struct HubInterruptRequest
{
    WDFREQUEST Request;
    struct _URB_BULK_OR_INTERRUPT_TRANSFER Urb;
    USBD_PIPE_HANDLE Pipe;

    /* The last completed transfer had no bit set; the next one asks UCX to pend */
    BOOLEAN LastInterruptWasEmpty;

    WDFMEMORY BitmapMemory;
    PUCHAR BitmapBuffer;
    ULONG BitmapMaxBytes;
    USHORT BitmapBytes;
    RTL_BITMAP Bitmap;
};

/* Hub class request codes */
#define HUB_REQUEST_GET_STATUS       0
#define HUB_REQUEST_CLEAR_FEATURE    1
#define HUB_REQUEST_SET_FEATURE      3
#define HUB_REQUEST_GET_DESCRIPTOR   6
#define HUB_REQUEST_CLEAR_TT_BUFFER  8
#define HUB_REQUEST_RESET_TT         9
#define HUB_REQUEST_SET_HUB_DEPTH    12

/* Hub and port feature selectors (USB 2.0 11.24.2, USB 3.2 10.16.2) */
#define HUB_C_LOCAL_POWER            0
#define HUB_C_OVER_CURRENT           1
#define PORT_F_ENABLE                1
#define PORT_F_SUSPEND               2
#define PORT_F_RESET                 4
#define PORT_F_LINK_STATE            5
#define PORT_F_POWER                 8
#define PORT_C_CONNECTION            16
#define PORT_C_ENABLE                17
#define PORT_C_SUSPEND               18
#define PORT_C_OVER_CURRENT          19
#define PORT_C_RESET                 20
#define PORT_F_U1_TIMEOUT            23
#define PORT_F_U2_TIMEOUT            24
#define PORT_C_LINK_STATE            25
#define PORT_C_CONFIG_ERROR          26
#define PORT_F_REMOTE_WAKE_MASK      27
#define PORT_F_BH_RESET              28
#define PORT_C_BH_RESET              29

/* USB 3 link states in port status bits 8:5 */
#define LINK_U0                      0
#define LINK_U3                      3
#define LINK_SS_DISABLED             4
#define LINK_RX_DETECT               5
#define LINK_SS_INACTIVE             6
#define LINK_POLLING                 7
#define LINK_RECOVERY                8
#define LINK_COMPLIANCE              10
#define LINK_LOOPBACK                11

/* Port status and change word bits */
#define PS_CONNECTED        0x0001
#define PS_ENABLED          0x0002
#define PS_SUSPENDED        0x0004
#define PS_OVER_CURRENT     0x0008
#define PS_RESET            0x0010
#define PS_LINK_SHIFT       5
#define PS_LINK_MASK        0x01E0
#define PS_POWER_20         0x0100
#define PS_POWER_30         0x0200

#define PC_CONNECT          0x0001
#define PC_ENABLE           0x0002
#define PC_SUSPEND          0x0004
#define PC_OVER_CURRENT     0x0008
#define PC_RESET            0x0010
#define PC_BH_RESET         0x0020
#define PC_LINK_STATE       0x0040
#define PC_CONFIG_ERROR     0x0080

/* Hub status and change bits */
#define HS_LOCAL_POWER_LOST 0x0001
#define HS_OVER_CURRENT     0x0002

/* Failure reasons the hub keeps for the PnP problem text */
#define HUB_MSG_INVALID_PORT_STATUS     0x40020000
#define HUB_MSG_CONTROL_TRANSFER_FAILED 0x40020001
#define HUB_MSG_INTERRUPT_FAILED        0x40020002

/* Enumeration failure texts for a broken USB 3 link */

FORCEINLINE
USHORT
NTAPI
HubLinkState(
    _In_ USHORT Status)
{
    return (Status & PS_LINK_MASK) >> PS_LINK_SHIFT;
}

/* Transfer helpers shared by the port and device modules */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubReadPortStatusForBootDevice(
    _In_ HubChild* Child,
    _Out_ PULONG PortStatus);
