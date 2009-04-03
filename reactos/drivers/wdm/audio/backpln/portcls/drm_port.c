/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/drm_port.c
 * PURPOSE:         portcls drm port object
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IDrmPort2Vtbl *lpVtbl;
    LONG ref;
}IDrmPort2Impl;

ULONG
NTAPI
IDrmPort2_fnAddRef(
    IN IDrmPort2* iface)
{
    IDrmPort2Impl * This = (IDrmPort2Impl*)iface;

    DPRINT("IDrmPort2_AddRef: This %p\n", This);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IDrmPort2_fnRelease(
    IN IDrmPort2* iface)
{
    IDrmPort2Impl * This = (IDrmPort2Impl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

NTSTATUS
NTAPI
IDrmPort2_fnQueryInterface(
    IN IDrmPort2* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;
    IDrmPort2Impl * This = (IDrmPort2Impl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IDrmPort) ||
        IsEqualGUIDAligned(refiid, &IID_IDrmPort2) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = (PVOID)&This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IDrmPort2_QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
IDrmPort2_fnCreateContentMixed(
    IN IDrmPort2 * iface,
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId)
{
    return DrmCreateContentMixed(paContentId, cContentId, pMixedContentId);
}

NTSTATUS
NTAPI
IDrmPort2_fnDestroyContent(
    IN IDrmPort2 * iface,
    IN ULONG ContentId)
{
    return DrmDestroyContent(ContentId);
}

NTSTATUS
NTAPI
IDrmPort2_fnForwardContentToFileObject(
    IN IDrmPort2 * iface,
    IN ULONG ContentId,
    IN PFILE_OBJECT FileObject)
{
    return DrmForwardContentToFileObject(ContentId, FileObject);
}

NTSTATUS
NTAPI
IDrmPort2_fnForwardContentToInterface(
    IN IDrmPort2 * iface,
    IN ULONG ContentId,
    IN PUNKNOWN pUnknown,
    IN ULONG NumMethods)
{
    return DrmForwardContentToInterface(ContentId, pUnknown, NumMethods);
}

NTSTATUS
NTAPI
IDrmPort2_fnGetContentRights(
    IN IDrmPort2 * iface,
    IN ULONG ContentId,
    OUT PDRMRIGHTS  DrmRights)
{
    return DrmGetContentRights(ContentId, DrmRights);
}

NTSTATUS
NTAPI
IDrmPort2_fnAddContentHandlers(
    IN IDrmPort2 * iface,
    IN ULONG ContentId,
    IN PVOID * paHandlers,
    IN ULONG NumHandlers)
{
    return DrmAddContentHandlers(ContentId, paHandlers, NumHandlers);
}

NTSTATUS
NTAPI
IDrmPort2_fnForwardContentToDeviceObject(
    IN IDrmPort2 * iface,
    IN ULONG ContentId,
    IN PVOID Reserved,
    IN PCDRMFORWARD DrmForward)
{
    return DrmForwardContentToDeviceObject(ContentId, Reserved, DrmForward);
}

static IDrmPort2Vtbl vt_IDrmPort2 =
{
    /* IUnknown methods */
    IDrmPort2_fnQueryInterface,
    IDrmPort2_fnAddRef,
    IDrmPort2_fnRelease,
    IDrmPort2_fnCreateContentMixed,
    IDrmPort2_fnDestroyContent,
    IDrmPort2_fnForwardContentToFileObject,
    IDrmPort2_fnForwardContentToInterface,
    IDrmPort2_fnGetContentRights,
    IDrmPort2_fnAddContentHandlers,
    IDrmPort2_fnForwardContentToDeviceObject
};

NTSTATUS
NewIDrmPort(
    OUT PDRMPORT2 *OutPort)
{
    IDrmPort2Impl * This = AllocateItem(NonPagedPool, sizeof(IDrmPort2Impl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IDrmPort2;
    This->ref = 1;

    *OutPort = (PDRMPORT2)&This->lpVtbl;
    return STATUS_SUCCESS;
}
