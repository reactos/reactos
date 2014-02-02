/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/drm.cpp
 * PURPOSE:         portcls drm functions
 * PROGRAMMER:      Andrew Greenwood
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

NTSTATUS
NTAPI
PcAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmAddContentHandlers(ContentId, paHandlers, NumHandlers);
}

NTSTATUS
NTAPI
PcCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmCreateContentMixed(paContentId, cContentId, pMixedContentId);
}

NTSTATUS
NTAPI
PcDestroyContent(
    IN  ULONG ContentId)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmDestroyContent(ContentId);
}

NTSTATUS
NTAPI
PcForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmForwardContentToDeviceObject(ContentId, Reserved, DrmForward);
}

NTSTATUS
NTAPI
PcForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmForwardContentToFileObject(ContentId, FileObject);
}

NTSTATUS
NTAPI
PcForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods)
{
    return DrmForwardContentToInterface(ContentId, pUnknown, NumMethods);
}

NTSTATUS
NTAPI
PcGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    return DrmGetContentRights(ContentId, DrmRights);
}
