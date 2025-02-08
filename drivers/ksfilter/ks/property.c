/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/event.c
 * PURPOSE:         KS property handling functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

/* SEH support with PSEH */
#include <pseh/pseh2.h>

#define NDEBUG
#include <debug.h>

const GUID KSPROPTYPESETID_General = {0x97E99BA0L, 0xBDEA, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

NTSTATUS
FindPropertyHandler(
    IN PIO_STATUS_BLOCK IoStatus,
    IN  const KSPROPERTY_SET* PropertySet,
    IN ULONG PropertySetCount,
    IN PKSPROPERTY Property,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    OUT PVOID OutputBuffer,
    OUT PFNKSHANDLER *PropertyHandler,
    OUT PKSPROPERTY_SET * Set,
    OUT PKSPROPERTY_ITEM *PropertyItem)
{
    ULONG Index, ItemIndex;
    PULONG Flags;
    PKSPROPERTY_DESCRIPTION Description;

    for(Index = 0; Index < PropertySetCount; Index++)
    {
        ASSERT(PropertySet[Index].Set);

        if (IsEqualGUIDAligned(&Property->Set, PropertySet[Index].Set))
        {
            for(ItemIndex = 0; ItemIndex < PropertySet[Index].PropertiesCount; ItemIndex++)
            {

                /* store property set */
                *Set = (PKSPROPERTY_SET)&PropertySet[Index];
                *PropertyItem = (PKSPROPERTY_ITEM)&PropertySet[Index].PropertyItem[ItemIndex];


                if (PropertySet[Index].PropertyItem[ItemIndex].PropertyId == Property->Id)
                {
                    if (Property->Flags & KSPROPERTY_TYPE_BASICSUPPORT)
                    {
                        if (sizeof(ULONG) > OutputBufferLength)
                        {
                            /* too small buffer */
                            return STATUS_INVALID_PARAMETER;
                        }

                        /* get output buffer */
                        Flags = (PULONG)OutputBuffer;

                        /* clear flags */
                        *Flags = 0;

                        IoStatus->Information = sizeof(ULONG);

                        if (PropertySet[Index].PropertyItem[ItemIndex].SupportHandler)
                        {
                            /* use support handler from driver */
                            *PropertyHandler = PropertySet[Index].PropertyItem[ItemIndex].SupportHandler;
                            return STATUS_SUCCESS;
                        }

                        if (PropertySet[Index].PropertyItem[ItemIndex].GetSupported)
                            *Flags |= KSPROPERTY_TYPE_GET;

                        if (PropertySet[Index].PropertyItem[ItemIndex].SetSupported)
                            *Flags |= KSPROPERTY_TYPE_SET;

                        if (OutputBufferLength >= sizeof(KSPROPERTY_DESCRIPTION))
                        {
                            /* get output buffer */
                            Description = (PKSPROPERTY_DESCRIPTION)OutputBuffer;

                            /* store result */
                            Description->DescriptionSize = sizeof(KSPROPERTY_DESCRIPTION);
                            Description->PropTypeSet.Set = KSPROPTYPESETID_General;
                            Description->PropTypeSet.Id = 0;
                            Description->PropTypeSet.Flags = 0;
                            Description->MembersListCount = 0;
                            Description->Reserved = 0;

                            IoStatus->Information = sizeof(KSPROPERTY_DESCRIPTION);
                        }
                        return STATUS_SUCCESS;
                    }

                    if (PropertySet[Index].PropertyItem[ItemIndex].MinProperty > InputBufferLength)
                    {
                        /* too small input buffer */
                        IoStatus->Information = PropertySet[Index].PropertyItem[ItemIndex].MinProperty;
                        return STATUS_INVALID_PARAMETER;
                    }

                    if (PropertySet[Index].PropertyItem[ItemIndex].MinData > OutputBufferLength)
                    {
                        /* too small output buffer */
                        IoStatus->Information = PropertySet[Index].PropertyItem[ItemIndex].MinData;
                        return STATUS_MORE_ENTRIES;
                    }


                    if (Property->Flags & KSPROPERTY_TYPE_SET)
                    {
                        /* store property handler */
                        *PropertyHandler = PropertySet[Index].PropertyItem[ItemIndex].SetPropertyHandler;
                        return STATUS_SUCCESS;
                    }

                    if (Property->Flags & KSPROPERTY_TYPE_GET)
                    {
                        /* store property handler */
                        *PropertyHandler = PropertySet[Index].PropertyItem[ItemIndex].GetPropertyHandler;
                        return STATUS_SUCCESS;
                    }


                }
            }
        }
    }
    return STATUS_NOT_FOUND;
}


NTSTATUS
KspPropertyHandler(
    IN PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG PropertyItemSize OPTIONAL)
{
    PKSPROPERTY Property;
    PKSPROPERTY_ITEM PropertyItem;
    PKSPROPERTY_SET Set;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PFNKSHANDLER PropertyHandler = NULL;
    ULONG Index, InputBufferLength, OutputBufferLength, TotalSize;
    LPGUID Guid;
    //UNICODE_STRING GuidBuffer;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get parameters */
    OutputBufferLength = (IoStack->Parameters.DeviceIoControl.OutputBufferLength + 7) & ~7;
    InputBufferLength = IoStack->Parameters.DeviceIoControl.InputBufferLength;

    /* check for invalid buffer length size */
    if (OutputBufferLength < IoStack->Parameters.DeviceIoControl.OutputBufferLength)
    {
        /* unsigned overflow */
        return STATUS_INVALID_BUFFER_SIZE;
    }

    /* check for integer overflow */
    if (InputBufferLength + OutputBufferLength < IoStack->Parameters.DeviceIoControl.OutputBufferLength)
    {
        /* overflow */
        return STATUS_INVALID_BUFFER_SIZE;
    }

    /* check if inputbuffer at least holds KSPROPERTY item */
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY))
    {
        /* invalid parameter */
        return STATUS_INVALID_BUFFER_SIZE;
    }

    /* get total size */
    TotalSize = InputBufferLength + OutputBufferLength;

    /* get input property request */
    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    /* have the parameters been checked yet */
    if (!Irp->AssociatedIrp.SystemBuffer)
    {
        /* is it from user mode */
        if (Irp->RequestorMode == UserMode)
        {
            /* probe user buffer */
            ProbeForRead(IoStack->Parameters.DeviceIoControl.Type3InputBuffer, InputBufferLength, 1);
        }

        /* do we have an allocator */
        if ((Allocator) && (Property->Flags & (KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET)))
        {
            /* call allocator */
            Status = Allocator(Irp, TotalSize, (Property->Flags & KSPROPERTY_TYPE_GET));

            /* check for success */
            if (!NT_SUCCESS(Status))
                return Status;
        }
        else
        {
            /* allocate buffer */
            Irp->AssociatedIrp.SystemBuffer = AllocateItem(NonPagedPool, TotalSize);

            /* sanity check */
            ASSERT(Irp->AssociatedIrp.SystemBuffer != NULL);

            /* mark irp as buffered so that changes the stream headers are propagated back */
            Irp->Flags |= IRP_DEALLOCATE_BUFFER | IRP_BUFFERED_IO;
        }

        /* now copy the buffer */
        RtlCopyMemory((PVOID)((ULONG_PTR)Irp->AssociatedIrp.SystemBuffer + OutputBufferLength), IoStack->Parameters.DeviceIoControl.Type3InputBuffer, InputBufferLength);

        /* use new property buffer */
        Property = (PKSPROPERTY)((ULONG_PTR)Irp->AssociatedIrp.SystemBuffer + OutputBufferLength);

        /* is it a set operation */
        if (Property->Flags & KSPROPERTY_TYPE_SET)
        {
            /* for set operations, the output parameters need to be copied */
            if (Irp->RequestorMode == UserMode)
            {
                /* probe user parameter */
                ProbeForRead(Irp->UserBuffer, IoStack->Parameters.DeviceIoControl.OutputBufferLength, 1);
            }

            /* copy parameters, needs un-aligned parameter length */
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Irp->UserBuffer, IoStack->Parameters.DeviceIoControl.OutputBufferLength);
        }

        /* is there an output buffer */
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength)
        {
            /* is it from user mode */
            if (Irp->RequestorMode == UserMode)
            {
                _SEH2_TRY
                {
                    /* probe buffer for writing */
                    ProbeForWrite(Irp->UserBuffer, IoStack->Parameters.DeviceIoControl.OutputBufferLength, 1);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    _SEH2_YIELD(return _SEH2_GetExceptionCode());
                }
                _SEH2_END;
            }

            if (!Allocator || !(Property->Flags & KSPROPERTY_TYPE_GET))
            {
                /* it is an input operation */
                Irp->Flags |= IRP_INPUT_OPERATION;
            }
        }
    }
    else
    {
        /* use new property buffer */
        Property = (PKSPROPERTY)((ULONG_PTR)Irp->AssociatedIrp.SystemBuffer + OutputBufferLength);
    }

    //RtlStringFromGUID(&Property->Set, &GuidBuffer);

    //DPRINT("KspPropertyHandler Irp %p PropertySetsCount %u PropertySet %p Allocator %p PropertyItemSize %u ExpectedPropertyItemSize %u\n", Irp, PropertySetsCount, PropertySet, Allocator, PropertyItemSize, sizeof(KSPROPERTY_ITEM));
    //DPRINT("PropertyId %lu PropertyFlags %x Guid %S\n", Property->Id, Property->Flags, GuidBuffer.Buffer);

    //RtlFreeUnicodeString(&GuidBuffer);

    /* sanity check */
    ASSERT(PropertyItemSize == 0 || PropertyItemSize == sizeof(KSPROPERTY_ITEM));

    /* find the property handler */
    Status = FindPropertyHandler(&Irp->IoStatus, PropertySet, PropertySetsCount, Property, InputBufferLength, OutputBufferLength, Irp->AssociatedIrp.SystemBuffer, &PropertyHandler, &Set, &PropertyItem);

    if (NT_SUCCESS(Status) && PropertyHandler)
    {
        /* store set */
        KSPROPERTY_SET_IRP_STORAGE(Irp) = Set;

        /* are any custom property item sizes used */
        if (PropertyItemSize)
        {
            /* store custom property item */
            KSPROPERTY_ITEM_IRP_STORAGE(Irp) = PropertyItem;
        }

        _SEH2_TRY
        {
            /* call property handler */
            Status = PropertyHandler(Irp, Property, (OutputBufferLength > 0 ? Irp->AssociatedIrp.SystemBuffer : NULL));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            /* output buffer is too small */
            if (Allocator)
            {
                /* allocate the requested amount */
                Status = Allocator(Irp, (ULONG)Irp->IoStatus.Information, FALSE);

                /* check if the block was allocated */
                if (!NT_SUCCESS(Status))
                {
                    /* no memory */
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                _SEH2_TRY
                {
                    /* re-call property handler */
                    Status = PropertyHandler(Irp, Property, Irp->AssociatedIrp.SystemBuffer);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status =  _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
        }
    }
    else if (IsEqualGUIDAligned(&Property->Set, &GUID_NULL) && Property->Id == 0 && (Property->Flags & KSPROPERTY_TYPE_SETSUPPORT) == KSPROPERTY_TYPE_SETSUPPORT)
    {
        // store output size
        Irp->IoStatus.Information = sizeof(GUID) * PropertySetsCount;
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GUID) * PropertySetsCount)
        {
            // buffer too small
            return STATUS_MORE_ENTRIES;
        }

        // get output buffer
        Guid = (LPGUID)Irp->AssociatedIrp.SystemBuffer;

       // copy property guids from property sets
       for(Index = 0; Index < PropertySetsCount; Index++)
       {
           RtlMoveMemory(&Guid[Index], PropertySet[Index].Set, sizeof(GUID));
       }
       Status = STATUS_SUCCESS;
    }

    /* done */
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsPropertyHandler(
    IN  PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet)
{
    return KspPropertyHandler(Irp, PropertySetsCount, PropertySet, NULL, 0);
}


/*
    @implemented
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsPropertyHandlerWithAllocator(
    _In_ PIRP Irp,
    _In_ ULONG PropertySetsCount,
    _In_reads_(PropertySetsCount) const KSPROPERTY_SET* PropertySet,
    _In_opt_ PFNKSALLOCATOR Allocator,
    _In_opt_ ULONG PropertyItemSize)
{
    return KspPropertyHandler(Irp, PropertySetsCount, PropertySet, Allocator, PropertyItemSize);
}

NTSTATUS
FindFastPropertyHandler(
    IN ULONG FastIoCount,
    IN const KSFASTPROPERTY_ITEM * FastIoTable,
    IN PKSPROPERTY PropertyId,
    OUT PFNKSFASTHANDLER * FastPropertyHandler)
{
    ULONG Index;

    /* iterate through all items */
    for(Index = 0; Index < FastIoCount; Index++)
    {
        if (PropertyId->Id == FastIoTable[Index].PropertyId)
        {
            if (PropertyId->Flags & KSPROPERTY_TYPE_SET)
            {
                if (FastIoTable[Index].SetSupported)
                {
                    *FastPropertyHandler = FastIoTable[Index].SetPropertyHandler;
                    return STATUS_SUCCESS;
                }
            }

            if (PropertyId->Flags & KSPROPERTY_TYPE_GET)
            {
                if (FastIoTable[Index].GetSupported)
                {
                    *FastPropertyHandler = FastIoTable[Index].GetPropertyHandler;
                    return STATUS_SUCCESS;
                }
            }
        }

    }
    /* no fast property handler found */
    return STATUS_NOT_FOUND;
}


/*
    @implemented
*/
KSDDKAPI
BOOLEAN
NTAPI
KsFastPropertyHandler(
    IN  PFILE_OBJECT FileObject,
    IN  PKSPROPERTY UNALIGNED Property,
    IN  ULONG PropertyLength,
    IN  OUT PVOID UNALIGNED Data,
    IN  ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet)
{
    KSPROPERTY PropRequest;
    KPROCESSOR_MODE Mode;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;
    PFNKSFASTHANDLER FastPropertyHandler;

    if (PropertyLength < sizeof(KSPROPERTY))
    {
        /* invalid request */
        return FALSE;
    }

    /* get previous mode */
    Mode = ExGetPreviousMode();

    if (Mode == KernelMode)
    {
        /* just copy it */
        RtlMoveMemory(&PropRequest, Property, sizeof(KSPROPERTY));
    }
    else
    {
        /* need to probe the buffer */
        _SEH2_TRY
        {
            ProbeForRead(Property, sizeof(KSPROPERTY), sizeof(UCHAR));
            RtlMoveMemory(&PropRequest, Property, sizeof(KSPROPERTY));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Exception, get the error code */
            Status = _SEH2_GetExceptionCode();
        }_SEH2_END;

        if (!NT_SUCCESS(Status))
            return FALSE;
    }

    /* are there any property sets provided */
    if (PropertySetsCount)
    {
        /* iterate through all property sets count */
        Index = 0;
        do
        {
            /* does the property id match */
            if (IsEqualGUIDAligned(PropertySet[Index].Set, &PropRequest.Set))
            {
                /* try to find a fast property handler */
                Status = FindFastPropertyHandler(PropertySet[Index].FastIoCount, PropertySet[Index].FastIoTable, &PropRequest, &FastPropertyHandler);

                if (NT_SUCCESS(Status))
                {
                    /* call fast property handler */
                    ASSERT(PropertyLength == sizeof(KSPROPERTY)); /* FIXME check if property length is bigger -> copy params */
                    ASSERT(Mode == KernelMode); /* FIXME need to probe usermode output buffer */
                    return FastPropertyHandler(FileObject, &PropRequest, sizeof(KSPROPERTY), Data, DataLength, IoStatus);
                }
            }
            /* move to next item */
            Index++;
        }while(Index < PropertySetsCount);
    }
    return FALSE;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSpecificProperty(
    IN  PIRP Irp,
    IN  PFNKSHANDLER Handler)
{
    PIO_STACK_LOCATION IoStack;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    return Handler(Irp, IoStack->Parameters.DeviceIoControl.Type3InputBuffer, Irp->UserBuffer);
}

