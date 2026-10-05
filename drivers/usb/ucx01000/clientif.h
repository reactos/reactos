/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USBD client handles and the USBDI bus interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* What GetUSBDIVersion reports; newer than the USBDI_VERSION in usb.h */
#define UCX_USBDI_VERSION 0x600

/** One client's USBD registration; outlives the USB device it was made against. */
class UcxUsbdHandle
{
public:
    _IRQL_requires_(PASSIVE_LEVEL)
    static
    NTSTATUS
    CreateFromQuery(
        _In_ UcxRootHub* RootHub,
        _In_ UcxUsbDevice* Device,
        _Inout_ PUSBD_CLIENT_INTERFACE Interface);

    VOID
    Unregister();

    static
    UcxUsbdHandle*
    FromClientHandle(
        _In_ USBD_CLIENT_HANDLE Handle)
    {
        return (UcxUsbdHandle*)Handle;
    }

    /* XRB allocation */

    NTSTATUS
    AllocateXrb(
        _In_ ULONG Size,
        _In_ ULONG Type,
        _Out_ UcxXrbPreamble** Xrb);

    VOID
    TrackXrb(
        _In_ UcxXrbPreamble* Xrb);

public:
    ULONG m_ContractVersion;
    PDEVICE_OBJECT m_ClientDeviceObject;
    ULONG m_PoolTag;
    PVOID m_ClientContext;
    UcxUsbDevice* m_Device;

    /* Still valid once the device is gone */
    UcxController* m_Controller;
    WDFMEMORY m_Memory;
    WDFDEVICE m_RootHubPdo;
    UcxRootHub* m_RootHub;

    /* On the USB device's handle list */
    LIST_ENTRY m_DeviceLink;

    /* Device delete already dropped the client device object reference */
    BOOLEAN m_Orphaned;

    /* Storage deleted; XRBs still out keep it readable through their references */
    BOOLEAN m_Unregistered;

    BOOLEAN m_VerifierEnabled;
    ULONG m_VerifierFailRegistration;
    ULONG m_VerifierFailChainedMdl;
    ULONG m_VerifierFailStaticStreamSupport;
    ULONG m_VerifierStaticStreamCountOverride;
    ULONG m_VerifierFailEnableStaticStreams;
    ULONG m_VerifierFailSecureTransfer;
    ULONG m_VerifierFailEndpointOffload;

    /* Granted through QUERY_USB_CAPABILITY */
    BOOLEAN m_StreamsGranted;
    BOOLEAN m_ChainedMdlGranted;
    ULONG m_GrantedStreams;

    /* Every live XRB, kept only when verifying */
    BOOLEAN m_TrackXrbs;
    KSPIN_LOCK m_XrbLock;
    LIST_ENTRY m_XrbList;
};

/** At device delete: what clients failed to unregister. */
VOID
NTAPI
UcxDropLeakedUsbdHandles(
    _In_ UcxUsbDevice* Device);

/** TRUE when any client registered against the device runs with USB verifier. */
BOOLEAN
NTAPI
UcxAnyUsbdHandleHasVerifier(
    _In_ UcxUsbDevice* Device);
