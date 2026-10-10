/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX static streams object interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "ucxglobals.h"
#include "ucxfuncenum.h"
#include "ucxobjects.h"

WDF_EXTERN_C_START

typedef struct _STREAM_INFO
{
    ULONG Size;
    WDFQUEUE WdfQueue;
    ULONG StreamId;
} STREAM_INFO, *PSTREAM_INFO;

FORCEINLINE
VOID
NTAPI
STREAM_INFO_INIT(
    _Out_ PSTREAM_INFO StreamInfo,
    _In_ WDFQUEUE WdfQueue,
    _In_ ULONG StreamId)
{
    RtlZeroMemory(StreamInfo, sizeof(*StreamInfo));
    StreamInfo->Size = sizeof(*StreamInfo);
    StreamInfo->WdfQueue = WdfQueue;
    StreamInfo->StreamId = StreamId;
}

/* Class entry points */

typedef
VOID
(NTAPI *PFN_UCXSTATICSTREAMSSETSTREAMINFO)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXSSTREAMS StaticStreams,
    _In_ PSTREAM_INFO StreamInfo);

typedef
_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXSTATICSTREAMSCREATE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _Inout_ PUCXSSTREAMS_INIT *StaticStreamsInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXSSTREAMS *StaticStreams);

FORCEINLINE
VOID
NTAPI
UcxStaticStreamsSetStreamInfo(
    _In_ UCXSSTREAMS StaticStreams,
    _In_ PSTREAM_INFO StreamInfo)
{
    PFN_UCXSTATICSTREAMSSETSTREAMINFO SetStreamInfo;

    SetStreamInfo = UCX_BOUND_FUNCTION(PFN_UCXSTATICSTREAMSSETSTREAMINFO,
                                       UcxStaticStreamsSetStreamInfoTableIndex);
    SetStreamInfo(UcxDriverGlobals, StaticStreams, StreamInfo);
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
FORCEINLINE
NTSTATUS
NTAPI
UcxStaticStreamsCreate(
    _In_ UCXENDPOINT Endpoint,
    _Inout_ PUCXSSTREAMS_INIT *StaticStreamsInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXSSTREAMS *StaticStreams)
{
    PFN_UCXSTATICSTREAMSCREATE Create;

    Create = UCX_BOUND_FUNCTION(PFN_UCXSTATICSTREAMSCREATE, UcxStaticStreamsCreateTableIndex);
    return Create(UcxDriverGlobals, Endpoint, StaticStreamsInit, Attributes, StaticStreams);
}

WDF_EXTERN_C_END
