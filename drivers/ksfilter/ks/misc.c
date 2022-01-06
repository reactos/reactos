/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/misc.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

#define TAG_KS 'ssKK'

VOID
CompleteRequest(
    PIRP Irp,
    CCHAR PriorityBoost)
{
    DPRINT("Completing IRP %p Status %x\n", Irp, Irp->IoStatus.Status);

    ASSERT(Irp->IoStatus.Status != STATUS_PENDING);


    IoCompleteRequest(Irp, PriorityBoost);
}

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    PVOID Item = ExAllocatePoolWithTag(PoolType, NumberOfBytes, TAG_KS);
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
KspCopyCreateRequest(
    IN PIRP Irp,
    IN LPWSTR ObjectClass,
    IN OUT PULONG Size,
    OUT PVOID * Result)
{
    PIO_STACK_LOCATION IoStack;
    SIZE_T ObjectLength, ParametersLength;
    PVOID Buffer;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get object class length */
    ObjectLength = (wcslen(ObjectClass) + 1) * sizeof(WCHAR);

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
    *Size = (ULONG)ParametersLength;

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
    @implemented
*/
PUNKNOWN
NTAPI
KsGetOuterUnknown(
    IN PVOID  Object)
{
    PKSBASIC_HEADER BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Object - sizeof(KSBASIC_HEADER));

    /* sanity check */
    ASSERT(BasicHeader->Type == KsObjectTypeDevice || BasicHeader->Type == KsObjectTypeFilterFactory ||
           BasicHeader->Type == KsObjectTypeFilter || BasicHeader->Type == KsObjectTypePin);

    /* return objects outer unknown */
    return BasicHeader->OuterUnknown;
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


