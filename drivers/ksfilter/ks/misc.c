/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/misc.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    PVOID Item = ExAllocatePool(PoolType, NumberOfBytes);
    if (!Item)
        return Item;

    RtlZeroMemory(Item, NumberOfBytes);
    return Item;
}

VOID
FreeItem(
    IN PVOID Item)
{

    ExFreePool(Item);
}

NTSTATUS
NTAPI
KspForwardIrpSynchronousCompletion(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    if (Irp->PendingReturned == TRUE)
    {
        KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
KspForwardIrpSynchronous(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* initialize the notification event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, KspForwardIrpSynchronousCompletion, (PVOID)&Event, TRUE, TRUE, TRUE);

    /* now call the driver */
    Status = IoCallDriver(DeviceHeader->KsDevice.NextDeviceObject, Irp);
    /* did the request complete yet */
    if (Status == STATUS_PENDING)
    {
        /* not yet, lets wait a bit */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }
    return Status;
}

NTSTATUS
KspCopyCreateRequest(
    IN PIRP Irp,
    IN LPWSTR ObjectClass,
    IN OUT PULONG Size,
    OUT PVOID * Result)
{
    PIO_STACK_LOCATION IoStack;
    ULONG ObjectLength, ParametersLength;
    PVOID Buffer;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get object class length */
    ObjectLength = (wcslen(ObjectClass) + 2) * sizeof(WCHAR);

    /* check for minium length requirement */
    if (ObjectLength  + *Size > IoStack->FileObject->FileName.MaximumLength)
        return STATUS_UNSUCCESSFUL;

    /* extract parameters length */
    ParametersLength = IoStack->FileObject->FileName.MaximumLength - ObjectLength;

    /* allocate buffer */
    Buffer = AllocateItem(NonPagedPool, ParametersLength);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* copy parameters */
    RtlMoveMemory(Buffer, &IoStack->FileObject->FileName.Buffer[ObjectLength / sizeof(WCHAR)], ParametersLength);

    /* store result */
    *Result = Buffer;
    *Size = ParametersLength;

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
PVOID
NTAPI
KsGetObjectFromFileObject(
    IN PFILE_OBJECT FileObject)
{
    PKSIOBJECT_HEADER ObjectHeader;

    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER)FileObject->FsContext2;

    /* return associated object */
    return ObjectHeader->ObjectType;
}

/*
    @implemented
*/
KSDDKAPI
KSOBJECTTYPE
NTAPI
KsGetObjectTypeFromFileObject(
    IN PFILE_OBJECT FileObject)
{
    PKSIOBJECT_HEADER ObjectHeader;

    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER)FileObject->FsContext2;
    /* return type */
    return ObjectHeader->Type;
}

/*
    @implemented
*/
KSOBJECTTYPE
NTAPI
KsGetObjectTypeFromIrp(
    IN PIRP  Irp)
{
    PKSIOBJECT_HEADER ObjectHeader;
    PIO_STACK_LOCATION IoStack;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;
    /* return type */
    return ObjectHeader->Type;
}

/*
    @unimplemented
*/
PUNKNOWN
NTAPI
KsGetOuterUnknown(
    IN PVOID  Object)
{
    UNIMPLEMENTED
    return NULL;

}

/*
    @implemented
*/
KSDDKAPI
PVOID
NTAPI
KsGetParent(
    IN PVOID Object)
{
    PKSBASIC_HEADER BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Object - sizeof(KSBASIC_HEADER));
    /* sanity check */
    ASSERT(BasicHeader->Parent.KsDevice != NULL);
    /* return object type */
    return (PVOID)BasicHeader->Parent.KsDevice;
}


