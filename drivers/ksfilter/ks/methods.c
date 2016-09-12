/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/methods.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
FindMethodHandler(
    IN PIO_STATUS_BLOCK IoStatus,
    IN  const KSMETHOD_SET* MethodSet,
    IN ULONG MethodSetCount,
    IN PKSMETHOD Method,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    OUT PVOID OutputBuffer,
    OUT PFNKSHANDLER *MethodHandler,
    OUT PKSMETHOD_SET * Set)
{
    ULONG Index, ItemIndex;

    for(Index = 0; Index < MethodSetCount; Index++)
    {
        ASSERT(MethodSet[Index].Set);

        if (IsEqualGUIDAligned(&Method->Set, MethodSet[Index].Set))
        {
            for(ItemIndex = 0; ItemIndex < MethodSet[Index].MethodsCount; ItemIndex++)
            {
                if (MethodSet[Index].MethodItem[ItemIndex].MethodId == Method->Id)
                {
                    if (MethodSet[Index].MethodItem[ItemIndex].MinMethod > InputBufferLength)
                    {
                        /* too small input buffer */
                        IoStatus->Information = MethodSet[Index].MethodItem[ItemIndex].MinMethod;
                        return STATUS_INVALID_PARAMETER;
                    }

                    if (MethodSet[Index].MethodItem[ItemIndex].MinData > OutputBufferLength)
                    {
                        /* too small output buffer */
                        IoStatus->Information = MethodSet[Index].MethodItem[ItemIndex].MinData;
                        return STATUS_MORE_ENTRIES;
                    }
                    if (Method->Flags & KSMETHOD_TYPE_BASICSUPPORT)
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

                        /* set flags flags */
                        *Flags = MethodSet[Index].MethodItem[ItemIndex].Flags;

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
                    *MethodHandler = MethodSet[Index].MethodItem[ItemIndex].MethodHandler;
                    *Set = (PKSMETHOD_SET)&MethodSet[Index];
                    return STATUS_SUCCESS;
                }
            }
        }
    }
    return STATUS_NOT_FOUND;
}

NTSTATUS
NTAPI
KspMethodHandlerWithAllocator(
    IN  PIRP Irp,
    IN  ULONG MethodSetsCount,
    IN  const KSMETHOD_SET *MethodSet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG MethodItemSize OPTIONAL)
{
    PKSMETHOD Method;
    PKSMETHOD_SET Set;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PFNKSHANDLER MethodHandler = NULL;
    ULONG Index;
    LPGUID Guid;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* check if inputbuffer at least holds KSMETHOD item */
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSMETHOD))
    {
        /* invalid parameter */
        Irp->IoStatus.Information = sizeof(KSPROPERTY);
        return STATUS_INVALID_BUFFER_SIZE;
    }

    /* FIXME probe the input / output buffer if from user mode */

    /* get input property request */
    Method = (PKSMETHOD)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

//    DPRINT("KspMethodHandlerWithAllocator Irp %p PropertySetsCount %u PropertySet %p Allocator %p PropertyItemSize %u ExpectedPropertyItemSize %u\n", Irp, PropertySetsCount, PropertySet, Allocator, PropertyItemSize, sizeof(KSPROPERTY_ITEM));

    /* sanity check */
    ASSERT(MethodItemSize == 0 || MethodItemSize == sizeof(KSMETHOD_ITEM));

    /* find the method handler */
    Status = FindMethodHandler(&Irp->IoStatus, MethodSet, MethodSetsCount, Method, IoStack->Parameters.DeviceIoControl.InputBufferLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength, Irp->UserBuffer, &MethodHandler, &Set);

    if (NT_SUCCESS(Status) && MethodHandler)
    {
        /* call method handler */
        KSMETHOD_SET_IRP_STORAGE(Irp) = Set;
        Status = MethodHandler(Irp, Method, Irp->UserBuffer);

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

                /* re-call method handler */
                Status = MethodHandler(Irp, Method, Irp->UserBuffer);
            }
        }
    }
    else if (IsEqualGUIDAligned(&Method->Set, &GUID_NULL) && Method->Id == 0 && Method->Flags == KSMETHOD_TYPE_SETSUPPORT)
    {
        // store output size
        Irp->IoStatus.Information = sizeof(GUID) * MethodSetsCount;
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GUID) * MethodSetsCount)
        {
            // buffer too small
            return STATUS_MORE_ENTRIES;
        }

        // get output buffer
        Guid = (LPGUID)Irp->UserBuffer;

       // copy property guids from property sets
       for(Index = 0; Index < MethodSetsCount; Index++)
       {
           RtlMoveMemory(&Guid[Index], MethodSet[Index].Set, sizeof(GUID));
       }
       return STATUS_SUCCESS;
    }

    /* done */
    return Status;
}

/*
    @implemented
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsMethodHandler(
    _In_ PIRP Irp,
    _In_ ULONG MethodSetsCount,
    _In_reads_(MethodSetsCount) const KSMETHOD_SET* MethodSet)
{
    return KspMethodHandlerWithAllocator(Irp, MethodSetsCount, MethodSet, NULL, 0);
}

/*
    @implemented
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsMethodHandlerWithAllocator(
    _In_ PIRP Irp,
    _In_ ULONG MethodSetsCount,
    _In_reads_(MethodSetsCount) const KSMETHOD_SET* MethodSet,
    _In_opt_ PFNKSALLOCATOR Allocator,
    _In_opt_ ULONG MethodItemSize)
{
    return KspMethodHandlerWithAllocator(Irp, MethodSetsCount, MethodSet, Allocator, MethodItemSize);
}


NTSTATUS
FindFastMethodHandler(
    IN ULONG FastIoCount,
    IN const KSFASTMETHOD_ITEM * FastIoTable,
    IN PKSMETHOD MethodId,
    OUT PFNKSFASTHANDLER * FastPropertyHandler)
{
    ULONG Index;

    /* iterate through all items */
    for(Index = 0; Index < FastIoCount; Index++)
    {
        if (MethodId->Id == FastIoTable[Index].MethodId)
        {
            if (FastIoTable[Index].MethodSupported)
            {
                *FastPropertyHandler = FastIoTable[Index].MethodHandler;
                return STATUS_SUCCESS;
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
KsFastMethodHandler(
    IN  PFILE_OBJECT FileObject,
    IN  PKSMETHOD UNALIGNED Method,
    IN  ULONG MethodLength,
    IN  OUT PVOID UNALIGNED Data,
    IN  ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  ULONG MethodSetsCount,
    IN  const KSMETHOD_SET* MethodSet)
{
    KSMETHOD MethodRequest;
    KPROCESSOR_MODE Mode;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;
    PFNKSFASTHANDLER FastMethodHandler;

    if (MethodLength < sizeof(KSPROPERTY))
    {
        /* invalid request */
        return FALSE;
    }

    /* get previous mode */
    Mode = ExGetPreviousMode();

    if (Mode == KernelMode)
    {
        /* just copy it */
        RtlMoveMemory(&MethodRequest, Method, sizeof(KSMETHOD));
    }
    else
    {
        /* need to probe the buffer */
        _SEH2_TRY
        {
            ProbeForRead(Method, sizeof(KSPROPERTY), sizeof(UCHAR));
            RtlMoveMemory(&MethodRequest, Method, sizeof(KSMETHOD));
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
    if (MethodSetsCount)
    {
        /* iterate through all property sets count */
        Index = 0;
        do
        {
            /* does the property id match */
            if (IsEqualGUIDAligned(MethodSet[Index].Set, &MethodRequest.Set))
            {
                /* try to find a fast property handler */
                Status = FindFastMethodHandler(MethodSet[Index].FastIoCount, MethodSet[Index].FastIoTable, &MethodRequest, &FastMethodHandler);

                if (NT_SUCCESS(Status))
                {
                    /* call fast property handler */
                    ASSERT(MethodLength == sizeof(KSMETHOD)); /* FIXME check if property length is bigger -> copy params */
                    ASSERT(Mode == KernelMode); /* FIXME need to probe usermode output buffer */
                    return FastMethodHandler(FileObject, &MethodRequest, sizeof(KSMETHOD), Data, DataLength, IoStatus);
                }
            }
            /* move to next item */
            Index++;
        }while(Index < MethodSetsCount);
    }
    return FALSE;
}
