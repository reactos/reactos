/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/drm.c
 * PURPOSE:         portcls drm functions
 * PROGRAMMER:      Andrew Greenwood
 */

#include "private.h"

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers)
{
    return DrmAddContentHandlers(ContentId, paHandlers, NumHandlers);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId)
{
    return DrmCreateContentMixed(paContentId, cContentId, pMixedContentId);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcDestroyContent(
    IN  ULONG ContentId)
{
    return DrmDestroyContent(ContentId);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward)
{
    return DrmForwardContentToDeviceObject(ContentId, Reserved, DrmForward);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject)
{
    return DrmForwardContentToFileObject(ContentId, FileObject);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods)
{
    return DrmForwardContentToInterface(ContentId, pUnknown, NumMethods);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PcGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights)
{
    return DrmGetContentRights(ContentId, DrmRights);
}
