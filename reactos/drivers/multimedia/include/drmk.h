/*
    ReactOS Kernel Streaming
    Digital Rights Management

    Author: Andrew Greenwood
*/

#ifndef DRMK_H
#define DRMK_H

#include <haxor.h>

typedef struct
{
    DWORD Flags;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PVOID Context;
} DRMFORWARD, *PDRMFORWARD, *PCDRMFORWARD;

typedef struct
{
    BOOL CopyProtect;
    ULONG Reserved;
    BOOL DigitalOutputDisable;
} DRMRIGHTS, *PDRMRIGHTS;


/* ===============================================================
    Digital Rights Management Functions
    TODO: Check calling convention
*/

NTSTATUS
DrmAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers);

NTSTATUS
DrmCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId);

NTSTATUS
DrmDestroyContent(
    IN  ULONG ContentId);

NTSTATUS
DrmForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward);

NTSTATUS
DrmForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject);

NTSTATUS
DrmForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods);

NTSTATUS
DrmGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights);


#endif
