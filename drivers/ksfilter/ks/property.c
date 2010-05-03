/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/event.c
 * PURPOSE:         KS property handling functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "priv.h"

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
    OUT PKSPROPERTY_SET * Set)
{
    ULONG Index, ItemIndex;

    for(Index = 0; Index < PropertySetCount; Index++)
    {
        ASSERT(PropertySet[Index].Set);

        if (IsEqualGUIDAligned(&Property->Set, PropertySet[Index].Set))
        {
            for(ItemIndex = 0; ItemIndex < PropertySet[Index].PropertiesCount; ItemIndex++)
            {
                if (PropertySet[Index].PropertyItem[ItemIndex].PropertyId == Property->Id)
                {
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
                    if (Property->Flags & KSPROPERTY_TYPE_BASICSUPPORT)
                    {
                        PULONG Flags;
                        PKSPROPERTY_DESCRIPTION Description;

                        if (sizeof(ULONG) > OutputBufferLength)
                        {
                            /* too small buffer */
                            return STATUS_INVALID_PARAMETER;
                        }

                        /* get output buffer */
                        Flags = (PULONG)OutputBuffer;

                        /* clear flags */
                        *Flags = 0;

                        if (PropertySet[Index].PropertyItem[ItemIndex].GetSupported)
                            *Flags |= KSPROPERTY_TYPE_GET;

                        if (PropertySet[Index].PropertyItem[ItemIndex].SetSupported)
                            *Flags |= KSPROPERTY_TYPE_SET;

                        IoStatus->Information = sizeof(ULONG);

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
                    if (Property->Flags & KSPROPERTY_TYPE_SET)
                        *PropertyHandler = PropertySet[Index].PropertyItem[ItemIndex].SetPropertyHandler;

                    if (Property->Flags & KSPROPERTY_TYPE_GET)
                        *PropertyHandler = PropertySet[Index].PropertyItem[ItemIndex].GetPropertyHandler;

                    *Set = (PKSPROPERTY_SET)&PropertySet[Index];
                    return STATUS_SUCCESS;
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
    PKSPROPERTY_SET Set;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PFNKSHANDLER PropertyHandler = NULL;
    ULONG Index;
    LPGUID Guid;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* check if inputbuffer at least holds KSPROPERTY item */
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY))
    {
        /* invalid parameter */
        Irp->IoStatus.Information = sizeof(KSPROPERTY);
        return STATUS_INVALID_BUFFER_SIZE;
    }

    /* FIXME probe the input / output buffer if from user mode */

    /* get input property request */
    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

//    DPRINT("KspPropertyHandler Irp %p PropertySetsCount %u PropertySet %p Allocator %p PropertyItemSize %u ExpectedPropertyItemSize %u\n", Irp, PropertySetsCount, PropertySet, Allocator, PropertyItemSize, sizeof(KSPROPERTY_ITEM));

    /* sanity check */
    ASSERT(PropertyItemSize == 0 || PropertyItemSize == sizeof(KSPROPERTY_ITEM));

    /* find the property handler */
    Status = FindPropertyHandler(&Irp->IoStatus, PropertySet, PropertySetsCount, Property, IoStack->Parameters.DeviceIoControl.InputBufferLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength, Irp->UserBuffer, &PropertyHandler, &Set);

    if (NT_SUCCESS(Status) && PropertyHandler)
    {
        /* call property handler */
        KSPROPERTY_SET_IRP_STORAGE(Irp) = Set;
        Status = PropertyHandler(Irp, Property, Irp->UserBuffer);

        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            /* output buffer is too small */
            if (Allocator)
            {
                /* allocate the requested amount */
                Status = Allocator(Irp, Irp->IoStatus.Information, FALSE);

                /* check if the block was allocated */
                if (!NT_SUCCESS(Status))
                {
                    /* no memory */
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                /* re-call property handler */
                Status = PropertyHandler(Irp, Property, Irp->UserBuffer);
            }
        }
    }
    else if (IsEqualGUIDAligned(&Property->Set, &GUID_NULL) && Property->Id == 0 && Property->Flags == KSPROPERTY_TYPE_SETSUPPORT)
    {
        // store output size
        Irp->IoStatus.Information = sizeof(GUID) * PropertySetsCount;
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GUID) * PropertySetsCount)
        {
            // buffer too small
            return STATUS_MORE_ENTRIES;
        }

        // get output buffer
        Guid = (LPGUID)Irp->UserBuffer;

       // copy property guids from property sets
       for(Index = 0; Index < PropertySetsCount; Index++)
       {
           RtlMoveMemory(&Guid[Index], PropertySet[Index].Set, sizeof(GUID));
       }
       return STATUS_SUCCESS;
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
KSDDKAPI
NTSTATUS
NTAPI
KsPropertyHandlerWithAllocator(
    IN  PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  PKSPROPERTY_SET PropertySet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG PropertyItemSize OPTIONAL)
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

