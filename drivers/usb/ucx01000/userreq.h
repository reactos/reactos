/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User mode and legacy host controller IOCTLs, and the WMI node info
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/** TRUE when UCX recognized and completed the request. */
BOOLEAN
NTAPI
UcxDispatchUserIoctl(
    _In_ WDFDEVICE Fdo,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode);

/* GUID_USB_WMI_NODE_INFO on the controller FDO */
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE UcxEvtWmiNodeInfoQueryInstance;
