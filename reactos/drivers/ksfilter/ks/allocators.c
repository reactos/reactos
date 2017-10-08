/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/allocators.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef enum
{
    ALLOCATOR_NPAGED_LOOKASIDE,
    ALLOCATOR_PAGED_LOOKASIDE,
    ALLOCATOR_CUSTOM
}ALLOCATOR_TYPE;

typedef enum
{
    ALLOCATOR_DEVICE_CONTROL,
    ALLOCATOR_DEVICE_CLOSE,
    ALLOCATOR_ALLOCATE,
    ALLOCATOR_FREE

}ALLOC_REQUEST;

typedef PVOID (*PFNKSPAGEDPOOLALLOCATE)(IN PPAGED_LOOKASIDE_LIST  Lookaside);
typedef PVOID (*PFNKSNPAGEDPOOLALLOCATE)(IN PNPAGED_LOOKASIDE_LIST  Lookaside);

typedef VOID (*PFNKSPAGEDPOOLFREE)(IN PPAGED_LOOKASIDE_LIST  Lookaside, IN PVOID  Entry);
typedef VOID (*PFNKSNPAGEDPOOLFREE)(IN PNPAGED_LOOKASIDE_LIST  Lookaside, IN PVOID  Entry);

typedef VOID (NTAPI *PFNKSNPAGEDPOOLDELETE)(IN PNPAGED_LOOKASIDE_LIST  Lookaside);
typedef VOID (NTAPI *PFNKSPAGEDPOOLDELETE)(IN PPAGED_LOOKASIDE_LIST  Lookaside);

typedef struct
{
    IKsAllocatorVtbl *lpVtbl;
    LONG ref;
    PKSIOBJECT_HEADER Header;
    ALLOCATOR_TYPE Type;

    KSSTREAMALLOCATOR_STATUS Status;

    union
    {
        NPAGED_LOOKASIDE_LIST NPagedList;
        PAGED_LOOKASIDE_LIST PagedList;
        PVOID CustomList;
    }u;

    union
    {
         PFNKSDEFAULTALLOCATE DefaultAllocate;
         PFNKSPAGEDPOOLALLOCATE PagedPool;
         PFNKSNPAGEDPOOLALLOCATE NPagedPool;
    }Allocate;

   union
    {
         PFNKSDEFAULTFREE DefaultFree;
         PFNKSPAGEDPOOLFREE PagedPool;
         PFNKSNPAGEDPOOLFREE NPagedPool;
    }Free;

    union
    {
        PFNKSDELETEALLOCATOR DefaultDelete;
        PFNKSNPAGEDPOOLDELETE NPagedPool;
        PFNKSPAGEDPOOLDELETE PagedPool;
    }Delete;

}ALLOCATOR, *PALLOCATOR;

/* use KSNAME_Allocator for IID_IKsAllocator */
const GUID IID_IKsAllocator =            {0x642F5D00L, 0x4791, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_StreamAllocator = {0x0cf6e4342, 0xec87, 0x11cf, {0xa1, 0x30, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4}};


NTSTATUS
NTAPI
IKsAllocator_Allocate(
    IN PFILE_OBJECT FileObject,
    PVOID *Frame);

VOID
NTAPI
IKsAllocator_FreeFrame(
    IN PFILE_OBJECT FileObject,
    PVOID Frame);


NTSTATUS
NTAPI
IKsAllocator_fnQueryInterface(
    IKsAllocator * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    PALLOCATOR This = (PALLOCATOR)CONTAINING_RECORD(iface, ALLOCATOR, lpVtbl);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, &IID_IKsAllocator))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IKsAllocator_fnAddRef(
    IKsAllocator * iface)
{
    PALLOCATOR This = (PALLOCATOR)CONTAINING_RECORD(iface, ALLOCATOR, lpVtbl);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsAllocator_fnRelease(
    IKsAllocator * iface)
{
    PALLOCATOR This = (PALLOCATOR)CONTAINING_RECORD(iface, ALLOCATOR, lpVtbl);

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

NTSTATUS
NTAPI
IKsAllocator_fnDeviceIoControl(
    IKsAllocator *iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PALLOCATOR This = (PALLOCATOR)CONTAINING_RECORD(iface, ALLOCATOR, lpVtbl);
    PIO_STACK_LOCATION IoStack;
    PKSSTREAMALLOCATOR_FUNCTIONTABLE FunctionTable;
    PKSSTREAMALLOCATOR_STATUS State;
    PKSPROPERTY Property;

    /* FIXME locks */

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode  != IOCTL_KS_PROPERTY)
    {
        /* only KSPROPERTY requests are supported */
        UNIMPLEMENTED;

        /* complete and forget irps */
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        CompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_NOT_IMPLEMENTED;
   }

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY))
    {
        /* invalid request */
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        CompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* check the request */
    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_StreamAllocator))
    {
        if (Property->Id == KSPROPERTY_STREAMALLOCATOR_FUNCTIONTABLE)
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSSTREAMALLOCATOR_FUNCTIONTABLE))
            {
                /* buffer too small */
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(KSSTREAMALLOCATOR_FUNCTIONTABLE);
                /* complete and forget irp */
                CompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_BUFFER_TOO_SMALL;
            }
            if (!(Property->Flags & KSPROPERTY_TYPE_GET))
            {
                /* only support retrieving the property */
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                /* complete and forget irp */
                CompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_UNSUCCESSFUL;
            }

            /* get output buffer */
            FunctionTable = (PKSSTREAMALLOCATOR_FUNCTIONTABLE)Irp->UserBuffer;

            FunctionTable->AllocateFrame = IKsAllocator_Allocate;
            FunctionTable->FreeFrame = IKsAllocator_FreeFrame;

            /* save result */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(KSSTREAMALLOCATOR_FUNCTIONTABLE);
            /* complete request */
            CompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
        else if (Property->Id == KSPROPERTY_STREAMALLOCATOR_STATUS)
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSPROPERTY_STREAMALLOCATOR_STATUS))
            {
                /* buffer too small */
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(KSPROPERTY_STREAMALLOCATOR_STATUS);
                /* complete and forget irp */
                CompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_BUFFER_TOO_SMALL;
            }
            if (!(Property->Flags & KSPROPERTY_TYPE_GET))
            {
                /* only support retrieving the property */
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                /* complete and forget irp */
                CompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_UNSUCCESSFUL;
            }

            /* get output buffer */
            State = (PKSSTREAMALLOCATOR_STATUS)Irp->UserBuffer;

            /* copy allocator status */
            RtlMoveMemory(State, &This->Status, sizeof(KSSTREAMALLOCATOR_STATUS));

            /* save result */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(KSSTREAMALLOCATOR_STATUS);

            /* complete request */
            CompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }
    }

    /* unhandled request */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    CompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
IKsAllocator_fnClose(
    IKsAllocator *iface)
{
    PALLOCATOR This = (PALLOCATOR)CONTAINING_RECORD(iface, ALLOCATOR, lpVtbl);

    /* FIXME locks */

    /* now close allocator */
    if (This->Type == ALLOCATOR_CUSTOM)
    {
        This->Delete.DefaultDelete(This->u.CustomList);
    }
    else if (This->Type == ALLOCATOR_NPAGED_LOOKASIDE)
    {
        This->Delete.NPagedPool(&This->u.NPagedList);
    }
    else if (This->Type == ALLOCATOR_PAGED_LOOKASIDE)
    {
        This->Delete.PagedPool(&This->u.PagedList);
    }

    /* free object header */
    KsFreeObjectHeader(&This->Header);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsAllocator_fnAllocateFrame(
    IKsAllocator *iface,
    IN PVOID * OutFrame)
{
    PVOID Frame = NULL;
    PALLOCATOR This = (PALLOCATOR)CONTAINING_RECORD(iface, ALLOCATOR, lpVtbl);

    /* FIXME locks */

    /* now allocate frame */
    if (This->Type == ALLOCATOR_CUSTOM)
    {
        Frame = This->Allocate.DefaultAllocate(This->u.CustomList);
    }
    else if (This->Type == ALLOCATOR_NPAGED_LOOKASIDE)
    {
        Frame = This->Allocate.NPagedPool(&This->u.NPagedList);
    }
    else if (This->Type == ALLOCATOR_PAGED_LOOKASIDE)
    {
        Frame = This->Allocate.PagedPool(&This->u.PagedList);
    }

    if (Frame)
    {
        *OutFrame = Frame;
        InterlockedIncrement((PLONG)&This->Status.AllocatedFrames);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
IKsAllocator_fnFreeFrame(
    IKsAllocator *iface,
    IN PVOID Frame)
{
    PALLOCATOR This = (PALLOCATOR)CONTAINING_RECORD(iface, ALLOCATOR, lpVtbl);

    /* now allocate frame */
    if (This->Type == ALLOCATOR_CUSTOM)
    {
        This->Free.DefaultFree(This->u.CustomList, Frame);
    }
    else if (This->Type == ALLOCATOR_NPAGED_LOOKASIDE)
    {
        This->Free.NPagedPool(&This->u.NPagedList, Frame);
    }
    else if (This->Type == ALLOCATOR_PAGED_LOOKASIDE)
    {
        This->Free.PagedPool(&This->u.PagedList, Frame);
    }
}


static IKsAllocatorVtbl vt_IKsAllocator =
{
    IKsAllocator_fnQueryInterface,
    IKsAllocator_fnAddRef,
    IKsAllocator_fnRelease,
    IKsAllocator_fnDeviceIoControl,
    IKsAllocator_fnClose,
    IKsAllocator_fnAllocateFrame,
    IKsAllocator_fnFreeFrame
};


/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateAllocator(
    IN  HANDLE ConnectionHandle,
    IN  PKSALLOCATOR_FRAMING AllocatorFraming,
    OUT PHANDLE AllocatorHandle)
{
    return KspCreateObjectType(ConnectionHandle,
                               KSSTRING_Allocator,
                               (PVOID)AllocatorFraming,
                               sizeof(KSALLOCATOR_FRAMING),
                               GENERIC_READ,
                               AllocatorHandle);
}

/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultAllocator(
    IN  PIRP Irp)
{
    return KsCreateDefaultAllocatorEx(Irp, NULL, NULL, NULL, NULL, NULL);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsValidateAllocatorCreateRequest(
    IN  PIRP Irp,
    OUT PKSALLOCATOR_FRAMING* OutAllocatorFraming)
{
    PKSALLOCATOR_FRAMING AllocatorFraming;
    ULONG Size;
    NTSTATUS Status;
    ULONG SupportedFlags;

    /* set minimum request size */
    Size = sizeof(KSALLOCATOR_FRAMING);

    Status = KspCopyCreateRequest(Irp,
                                  KSSTRING_Allocator,
                                  &Size,
                                  (PVOID*)&AllocatorFraming);

    if (!NT_SUCCESS(Status))
        return Status;

    /* allowed supported flags */
    SupportedFlags = (KSALLOCATOR_OPTIONF_COMPATIBLE | KSALLOCATOR_OPTIONF_SYSTEM_MEMORY |
                      KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER | KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | KSALLOCATOR_REQUIREMENTF_FRAME_INTEGRITY | 
                      KSALLOCATOR_REQUIREMENTF_MUST_ALLOCATE);


    if (!AllocatorFraming->FrameSize || (AllocatorFraming->OptionsFlags & (~SupportedFlags)))
    {
        FreeItem(AllocatorFraming);
        return STATUS_INVALID_PARAMETER;
    }

    /* store result */
    *OutAllocatorFraming = AllocatorFraming;

    return Status;
}

NTSTATUS
IKsAllocator_DispatchRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVOID Frame,
    IN ALLOC_REQUEST Request)
{
    PKSIOBJECT_HEADER Header;
    NTSTATUS Status;
    IKsAllocator * Allocator;

    /* sanity check */
    ASSERT(FileObject);

    /* get object header */
    Header = (PKSIOBJECT_HEADER)FileObject->FsContext2;

    /* get real allocator */
    Status = Header->Unknown->lpVtbl->QueryInterface(Header->Unknown, &IID_IKsAllocator, (PVOID*)&Allocator);

    if (!NT_SUCCESS(Status))
    {
        /* misbehaving object */
        return STATUS_UNSUCCESSFUL;
    }

    if (Request == ALLOCATOR_DEVICE_CONTROL)
    {
        /* dispatch request allocator */
        Status = Allocator->lpVtbl->DispatchDeviceIoControl(Allocator, DeviceObject, Irp);
    }
    else if (Request == ALLOCATOR_DEVICE_CLOSE)
    {
        /* delete allocator */
        Status = Allocator->lpVtbl->Close(Allocator);
    }
    else if (Request == ALLOCATOR_ALLOCATE)
    {
        /* allocate frame */
        Status = Allocator->lpVtbl->AllocateFrame(Allocator, (PVOID*)Frame);

    }else if (Request == ALLOCATOR_FREE)
    {
        /* allocate frame */
        Allocator->lpVtbl->FreeFrame(Allocator, Frame);
        Status = STATUS_SUCCESS;
    }

    /* release interface */
    Allocator->lpVtbl->Release(Allocator);

    return Status;
}

NTSTATUS
NTAPI
IKsAllocator_DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* dispatch request */
    Status = IKsAllocator_DispatchRequest(DeviceObject, IoStack->FileObject, Irp, NULL, ALLOCATOR_DEVICE_CONTROL);

    /* complete request */
    Irp->IoStatus.Status = Status;
    CompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
IKsAllocator_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* dispatch request */
    Status = IKsAllocator_DispatchRequest(DeviceObject, IoStack->FileObject, Irp, NULL, ALLOCATOR_DEVICE_CLOSE);

    /* complete request */
    Irp->IoStatus.Status = Status;
    CompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
IKsAllocator_Allocate(
    IN PFILE_OBJECT FileObject,
    PVOID *Frame)
{
    NTSTATUS Status;

    /* dispatch request */
    Status = IKsAllocator_DispatchRequest(NULL, FileObject, NULL, (PVOID)Frame, ALLOCATOR_ALLOCATE);

    return Status;
}

VOID
NTAPI
IKsAllocator_FreeFrame(
    IN PFILE_OBJECT FileObject,
    PVOID Frame)
{
    /* dispatch request */
    IKsAllocator_DispatchRequest(NULL, FileObject, NULL, Frame, ALLOCATOR_FREE);
}


static KSDISPATCH_TABLE DispatchTable =
{
    IKsAllocator_DispatchDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    IKsAllocator_DispatchClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastReadFailure,
};

/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultAllocatorEx(
    IN  PIRP Irp,
    IN  PVOID InitializeContext OPTIONAL,
    IN  PFNKSDEFAULTALLOCATE DefaultAllocate OPTIONAL,
    IN  PFNKSDEFAULTFREE DefaultFree OPTIONAL,
    IN  PFNKSINITIALIZEALLOCATOR InitializeAllocator OPTIONAL,
    IN  PFNKSDELETEALLOCATOR DeleteAllocator OPTIONAL)
{
    NTSTATUS Status;
    PKSALLOCATOR_FRAMING AllocatorFraming;
    PALLOCATOR Allocator;
    PVOID Ctx;

    /* first validate connect request */
    Status = KsValidateAllocatorCreateRequest(Irp, &AllocatorFraming);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;

    /* check the valid file alignment */
    if (AllocatorFraming->FileAlignment > (PAGE_SIZE-1))
    {
        FreeItem(AllocatorFraming);
        return STATUS_INVALID_PARAMETER;
    }

    /* allocate allocator struct */
    Allocator = AllocateItem(NonPagedPool, sizeof(ALLOCATOR));
    if (!Allocator)
    {
        FreeItem(AllocatorFraming);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* allocate object header */
    
    Status = KsAllocateObjectHeader((KSOBJECT_HEADER*)&Allocator->Header, 0, NULL, Irp, &DispatchTable);
    if (!NT_SUCCESS(Status))
    {
        FreeItem(AllocatorFraming);
        FreeItem(Allocator);
        return Status;
    }

    /* set allocator type in object header */
    Allocator->lpVtbl = &vt_IKsAllocator;
    Allocator->Header->Unknown = (PUNKNOWN)&Allocator->lpVtbl;
    Allocator->ref = 1;

    if (DefaultAllocate)
    {
        /* use external allocator */
        Allocator->Type  = ALLOCATOR_CUSTOM;
        Allocator->Allocate.DefaultAllocate = DefaultAllocate;
        Allocator->Free.DefaultFree = DefaultFree;
        Allocator->Delete.DefaultDelete = DeleteAllocator;
        Ctx = InitializeAllocator(InitializeContext, AllocatorFraming, &Allocator->u.CustomList);
        /* check for success */
        if (!Ctx)
        {
            KsFreeObjectHeader(Allocator->Header);
            FreeItem(Allocator);
            return Status;
        }
    }
    else if (AllocatorFraming->PoolType == NonPagedPool)
    {
        /* use non-paged pool allocator */
        Allocator->Type  = ALLOCATOR_NPAGED_LOOKASIDE;
        Allocator->Allocate.NPagedPool = ExAllocateFromNPagedLookasideList;
        Allocator->Free.NPagedPool = ExFreeToNPagedLookasideList;
        Allocator->Delete.NPagedPool = ExDeleteNPagedLookasideList;
        ExInitializeNPagedLookasideList(&Allocator->u.NPagedList, NULL, NULL, 0, AllocatorFraming->FrameSize, 0, 0);
    }
    else if (AllocatorFraming->PoolType == PagedPool)
    {
        /* use paged pool allocator */
        Allocator->Allocate.PagedPool = ExAllocateFromPagedLookasideList;
        Allocator->Free.PagedPool = ExFreeToPagedLookasideList;
        Allocator->Delete.PagedPool = ExDeletePagedLookasideList;
        Allocator->Type  = ALLOCATOR_PAGED_LOOKASIDE;
        ExInitializePagedLookasideList(&Allocator->u.PagedList, NULL, NULL, 0, AllocatorFraming->FrameSize, 0, 0);

    }

    /* backup allocator framing */
    RtlMoveMemory(&Allocator->Status.Framing, AllocatorFraming, sizeof(KSALLOCATOR_FRAMING));
    FreeItem(AllocatorFraming);

    return Status;
}

/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsValidateAllocatorFramingEx(
    IN  PKSALLOCATOR_FRAMING_EX Framing,
    IN  ULONG BufferSize,
    IN  const KSALLOCATOR_FRAMING_EX* PinFraming)
{
    if (BufferSize < sizeof(KSALLOCATOR_FRAMING_EX))
       return  STATUS_INVALID_DEVICE_REQUEST;

    /* verify framing */
    if ((Framing->FramingItem[0].Flags & KSALLOCATOR_FLAG_PARTIAL_READ_SUPPORT) &&
         Framing->OutputCompression.RatioNumerator != MAXULONG &&
         Framing->OutputCompression.RatioDenominator != 0 &&
         Framing->OutputCompression.RatioDenominator < Framing->OutputCompression.RatioNumerator)
    {
        /* framing request is ok */
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}
