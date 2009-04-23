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
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
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
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
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
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
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
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
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
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
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
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmGetContentRights(ContentId, DrmRights);
}
