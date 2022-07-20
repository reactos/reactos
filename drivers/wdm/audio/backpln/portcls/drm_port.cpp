/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/drm_port.cpp
 * PURPOSE:         portcls drm port object
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CDrmPort2 : public CUnknownImpl<IDrmPort2>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IDrmPort2;
    CDrmPort2(IUnknown *OuterUnknown){}
    virtual ~CDrmPort2(){}
};

NTSTATUS
NTAPI
CDrmPort2::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IDrmPort) ||
        IsEqualGUIDAligned(refiid, IID_IDrmPort2) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT("IDrmPort2_QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CDrmPort2::CreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmCreateContentMixed(paContentId, cContentId, pMixedContentId);
}

NTSTATUS
NTAPI
CDrmPort2::DestroyContent(
    IN ULONG ContentId)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmDestroyContent(ContentId);
}

NTSTATUS
NTAPI
CDrmPort2::ForwardContentToFileObject(
    IN ULONG ContentId,
    IN PFILE_OBJECT FileObject)
{
    return DrmForwardContentToFileObject(ContentId, FileObject);
}

NTSTATUS
NTAPI
CDrmPort2::ForwardContentToInterface(
    IN ULONG ContentId,
    IN PUNKNOWN pUnknown,
    IN ULONG NumMethods)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmForwardContentToInterface(ContentId, pUnknown, NumMethods);
}

NTSTATUS
NTAPI
CDrmPort2::GetContentRights(
    IN ULONG ContentId,
    OUT PDRMRIGHTS  DrmRights)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmGetContentRights(ContentId, DrmRights);
}

NTSTATUS
NTAPI
CDrmPort2::AddContentHandlers(
    IN ULONG ContentId,
    IN PVOID * paHandlers,
    IN ULONG NumHandlers)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmAddContentHandlers(ContentId, paHandlers, NumHandlers);
}

NTSTATUS
NTAPI
CDrmPort2::ForwardContentToDeviceObject(
    IN ULONG ContentId,
    IN PVOID Reserved,
    IN PCDRMFORWARD DrmForward)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmForwardContentToDeviceObject(ContentId, Reserved, DrmForward);
}

NTSTATUS
NewIDrmPort(
    OUT PDRMPORT2 *OutPort)
{
    CDrmPort2 * This = new(NonPagedPool, TAG_PORTCLASS)CDrmPort2(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    *OutPort = (PDRMPORT2)This;
    return STATUS_SUCCESS;
}


