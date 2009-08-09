/*
    ReactOS Kernel Streaming
    Digital Rights Management

    Please see COPYING in the top-level directory for license information.

    Author: Andrew Greenwood

    Notes:
    This is just a file containing stub functions. The DRMK kernel library
    deals with Digital Rights Management. This is not essential for the
    operation of audio/video (except in the cases where content has digital
    rights information) but is needed for linking with PORTCLS.
*/

#include <ntddk.h>
#include <portcls.h>
#include <debug.h>

/*
    Provide a driver interface consisting of functions for handling DRM
    protected content
*/
/*
 * @unimplemented
 */
NTAPI NTSTATUS
DrmAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    Creates a DRM content ID to identify a KS audio stream containing
    mixed content from several input streams.
*/
/*
 * @unimplemented
 */
NTAPI NTSTATUS
DrmCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    Deletes a DRM content ID.
*/
/*
 * @unimplemented
 */
NTAPI NTSTATUS
DrmDestroyContent(
    IN  ULONG ContentId)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    Authenticates a driver, then passes it the DRM content ID, along with
    the content rights which have been assigned to a stream.
*/
/*
 * @unimplemented
 */
NTAPI NTSTATUS
DrmForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    Obsolete because it forces the system to run at a lower DRM security
    level. Returns STATUS_NOT_IMPLEMENTED if a pin associated with
    FileObject doesnt support the rights assigned to ContentId.
*/
/*
 * @unimplemented
 */
NTAPI NTSTATUS
DrmForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    Similar to DrmForwardContentToDeviceObject, except this works with a driver
    object rather than just a driver.
*/
/*
 * @unimplemented
 */
NTAPI NTSTATUS
DrmForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    Retrieves DRM Content rights that have been assigend to a DRM Content ID.
*/
/*
 * @unimplemented
 */
NTAPI NTSTATUS
DrmGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

