/*
    ReactOS Kernel Streaming
    Port Class / Digital Rights Management

    Author: Andrew Greenwood

    Notes:
        These are convenience functions for accessing DRM facilities, as
        documented here:
        http://www.osronline.com/ddkx/stream/aud-prop_9f77.htm
*/

#include <portcls.h>
#include <drmk.h>

PORTCLASSAPI NTSTATUS NTAPI
PcAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers)
{
    return DrmAddContentHandlers(ContentId, paHandlers, NumHandlers);
}

PORTCLASSAPI NTSTATUS NTAPI
PcCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId)
{
    return DrmCreateContentMixed(paContentId, cContentId, pMixedContentId);
}

PORTCLASSAPI NTSTATUS NTAPI
PcDestroyContent(
    IN  ULONG ContentId)
{
    return DrmDestroyContent(ContentId);
}

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward)
{
    return DrmForwardContentToDeviceObject(ContentId, Reserved, DrmForward);
}

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject)
{
    return DrmForwardContentToFileObject(ContentId, FileObject);
}

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods)
{
    return DrmForwardContentToInterface(ContentId, pUnknown, NumMethods);
}

PORTCLASSAPI NTSTATUS NTAPI
PcGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights)
{
    return DrmGetContentRights(ContentId, DrmRights);
}
