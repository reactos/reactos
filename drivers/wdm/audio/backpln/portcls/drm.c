/*
    ReactOS Kernel Streaming
    Port Class / Digital Rights Management

    Author: Andrew Greenwood

    Notes:
        These are convenience functions for accessing DRM facilities, as
        documented here:
        http://www.osronline.com/ddkx/stream/aud-prop_9f77.htm
*/

#include "private.h"
#include <portcls.h>
#include <drmk.h>

NTSTATUS NTAPI
PcAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers)
{
    return DrmAddContentHandlers(ContentId, paHandlers, NumHandlers);
}

NTSTATUS NTAPI
PcCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId)
{
    return DrmCreateContentMixed(paContentId, cContentId, pMixedContentId);
}

NTSTATUS NTAPI
PcDestroyContent(
    IN  ULONG ContentId)
{
    return DrmDestroyContent(ContentId);
}

NTSTATUS NTAPI
PcForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward)
{
    return DrmForwardContentToDeviceObject(ContentId, Reserved, DrmForward);
}

NTSTATUS NTAPI
PcForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject)
{
    return DrmForwardContentToFileObject(ContentId, FileObject);
}

NTSTATUS NTAPI
PcForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods)
{
    return DrmForwardContentToInterface(ContentId, pUnknown, NumMethods);
}

NTSTATUS NTAPI
PcGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights)
{
    return DrmGetContentRights(ContentId, DrmRights);
}
