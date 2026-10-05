/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Per request context shared by every WDFREQUEST the driver handles
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Control transfers */

enum class XhciControlMechanism : ULONG
{
    NoData,
    Immediate,
    DoubleBuffer,
    Dma
};

enum class XhciControlCancel : ULONG
{
    Idle,
    Armed,
    CancelRoutinePending,
    Done
};

enum class XhciControlTimeout : ULONG
{
    Idle,
    Armed,
    TimerRoutinePending,
    Expired
};

/** Tracking record of one control request, kept in its request context. */
struct XhciControlTransfer
{
    LIST_ENTRY Link;
    WDFREQUEST Request;
    PURB Urb;
    XhciTransferRing* Ring;
    XhciControlMechanism Mechanism;
    PMDL Mdl;
    PVOID SystemBuffer;
    XhciDmaBuffer* DoubleBuffer;
    PSCATTER_GATHER_LIST SgList;
    ULONG BytesTotal;
    ULONG BytesTransferred;
    XhciControlCancel CancelState;
    XhciControlTimeout TimeoutState;
    NTSTATUS Status;
    ULONG CompletionCode;
    ULONG TdCount;
    ULONG EventsReceived;
    ULONG FirstIndex;               /**< Slot of the first TRB of the TD */
    ULONG EndIndex;                 /**< Enqueue index after the TD */
    ULONG TagIndex;
    BOOLEAN Initialized;
    BOOLEAN TagArmed;
    BOOLEAN DataIn;
};

/* Endpoint requests */

/** DEFAULT_ENDPOINT_UPDATE: Evaluate Context with its own input context. */
struct XhciDefaultEndpointUpdate
{
    XhciEndpoint* Endpoint;
    XhciDmaBuffer* InputContext;
    ULONG MaxPacketSize;
    XhciCommand Command;
};

/** Client endpoint reset: Reset Endpoint or the reconfigure that stands in for it. */
struct XhciEndpointReset
{
    XhciEndpoint* Endpoint;
    XhciDmaBuffer* InputContext;
    XhciCommand Command;
};

/** Static streams open or close: the streams record being switched to or from. */
struct XhciStreamsConfigure
{
    XhciStreams* Streams;
};

/* Context of every request on the controller WDFDEVICE; one member is in use at a time, owned by the handling module */
union XhciRequestData
{
    UCHAR Raw[1];

    /** ENDPOINTS_CONFIGURE: what went wrong before or during the first Configure Endpoint. */
    struct
    {
        BOOLEAN EnableFailed;
        BOOLEAN FirstCommandFailed;
    } EndpointsConfigure;

    XhciDefaultEndpointUpdate DefaultEndpointUpdate;
    XhciEndpointReset EndpointReset;
    XhciStreamsConfigure StreamsConfigure;

    XhciControlTransfer Control;
    XhciBulkTransfer Bulk;
    XhciIsochTransfer Isoch;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XhciRequestData, XhciGetRequestData);
