/*
 * PROJECT:     ReactOS USB 3 stack
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Client handle interface between usbd and the USB controller extension
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* {DEEE98EA-C0A1-42C3-9738-A04606C84E93} */
DEFINE_GUID(GUID_USBD_CLIENT_INTERFACE,
            0xdeee98ea, 0xc0a1, 0x42c3, 0x97, 0x38, 0xa0, 0x46, 0x06, 0xc8, 0x4e, 0x93);

#define USBD_CLIENT_INTERFACE_VERSION 0x602

DECLARE_HANDLE(USBD_CLIENT_HANDLE);

typedef
VOID
(NTAPI *PFN_USBD_CLIENT_UNREGISTER)(
    _In_ USBD_CLIENT_HANDLE Handle);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFN_USBD_CLIENT_XRB_ALLOCATE)(
    _In_ USBD_CLIENT_HANDLE Handle,
    _Out_ PURB* Urb);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFN_USBD_CLIENT_ISOCH_XRB_ALLOCATE)(
    _In_ USBD_CLIENT_HANDLE Handle,
    _In_ ULONG NumberOfIsochPackets,
    _Out_ PURB* Urb);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFN_USBD_CLIENT_SELECT_XRB_BUILD)(
    _In_ USBD_CLIENT_HANDLE Handle,
    _In_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    _In_ PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    _Out_ PURB* Urb);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFN_USBD_CLIENT_SELECT_INTERFACE_XRB_BUILD)(
    _In_ USBD_CLIENT_HANDLE Handle,
    _In_ USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    _In_ PUSBD_INTERFACE_LIST_ENTRY InterfaceListEntry,
    _Out_ PURB* Urb);

typedef
VOID
(NTAPI *PFN_USBD_CLIENT_XRB_FREE)(
    _In_ PURB Urb);

typedef struct _USBD_CLIENT_INTERFACE
{
    INTERFACE Header;

    /* Filled by the requester */
    ULONG ClientContractVersion;
    USBD_CLIENT_HANDLE Handle;
    PDEVICE_OBJECT DeviceObject;
    ULONG PoolTag;
    PVOID ClientContext;
    ULONG VerifierEnabled;
    ULONG VerifierFailRegistration;
    ULONG VerifierFailChainedMdlSupport;
    ULONG VerifierFailStaticStreamSupport;
    ULONG VerifierStaticStreamCountOverride;
    ULONG VerifierFailEnableStaticStreams;
    ULONG VerifierTrackXrbs;

    /* Filled by the controller extension */
    PFN_USBD_CLIENT_UNREGISTER Unregister;
    PFN_USBD_CLIENT_XRB_ALLOCATE AllocUrb;
    PFN_USBD_CLIENT_ISOCH_XRB_ALLOCATE AllocIsochUrb;
    PFN_USBD_CLIENT_SELECT_XRB_BUILD AllocSelectConfigUrb;
    PFN_USBD_CLIENT_SELECT_INTERFACE_XRB_BUILD AllocSelectInterfaceUrb;
    PFN_USBD_CLIENT_XRB_FREE ReleaseUrb;
} USBD_CLIENT_INTERFACE, *PUSBD_CLIENT_INTERFACE;

#ifdef _WIN64
C_ASSERT(sizeof(USBD_CLIENT_INTERFACE) == 0x98);
#else
C_ASSERT(sizeof(USBD_CLIENT_INTERFACE) == 0x58);
#endif

/* Version 0x603 appends secure isoch and endpoint offload entries */

#define USBD_CLIENT_INTERFACE_VERSION_603 0x603

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFN_USBD_CLIENT_OFFLOAD_SELECT_XRB_ALLOCATE)(
    _In_ USBD_CLIENT_HANDLE Handle,
    _In_ USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    _Inout_ PUSBD_INTERFACE_LIST_ENTRY InterfaceListEntry,
    _In_ ULONG OffloadedEndpoints,
    _Out_ PURB* Urb);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFN_USBD_CLIENT_OFFLOAD_NOTIFY_XRB_ALLOCATE)(
    _In_ USBD_CLIENT_HANDLE Handle,
    _In_ USBD_PIPE_HANDLE PipeHandle,
    _In_ ULONG Flags,
    _Out_ PURB* Urb);

typedef struct _USBD_CLIENT_INTERFACE_603
{
    USBD_CLIENT_INTERFACE Base;

    /* Present from size USBD_CLIENT_INTERFACE_603_SIZE_SECURE */
    PFN_USBD_CLIENT_ISOCH_XRB_ALLOCATE SecureIsochXrbAllocate;
    ULONG VerifierFailSecureTransferSupport;

    /* Present from sizeof(USBD_CLIENT_INTERFACE_603) */
    PFN_USBD_CLIENT_OFFLOAD_SELECT_XRB_ALLOCATE OffloadSelectInterfaceXrbAllocate;
    ULONG VerifierFailEndpointOffload;
    PFN_USBD_CLIENT_OFFLOAD_NOTIFY_XRB_ALLOCATE OffloadNotificationXrbAllocate;
    PVOID Reserved;
} USBD_CLIENT_INTERFACE_603, *PUSBD_CLIENT_INTERFACE_603;

#define USBD_CLIENT_INTERFACE_603_SIZE_SECURE \
    (FIELD_OFFSET(USBD_CLIENT_INTERFACE_603, OffloadSelectInterfaceXrbAllocate))

#ifdef _WIN64
C_ASSERT(USBD_CLIENT_INTERFACE_603_SIZE_SECURE == 0xA8);
C_ASSERT(sizeof(USBD_CLIENT_INTERFACE_603) == 0xC8);
#else
C_ASSERT(USBD_CLIENT_INTERFACE_603_SIZE_SECURE == 0x60);
C_ASSERT(sizeof(USBD_CLIENT_INTERFACE_603) == 0x70);
#endif

#ifdef __cplusplus
}
#endif
