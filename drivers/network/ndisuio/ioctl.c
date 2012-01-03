/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        ioctl.c
 * PURPOSE:     IOCTL handling
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "ndisuio.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
SetAdapterOid(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDISUIO_SET_OID SetOidRequest;
    NDIS_REQUEST NdisRequest;
    ULONG RequestLength;
    NDIS_STATUS Status;
    
    Irp->IoStatus.Information = 0;
    
    SetOidRequest = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    RequestLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    if (QueryOidRequest && RequestLength >= sizeof(NDIS_OID))
    {
        /* Setup the NDIS request */
        NdisRequest.RequestType = NdisRequestSetInformation;
        NdisRequest.Oid = SetOidRequest->Oid;
        NdisRequest.InformationBuffer = SetOidRequest->Data;
        NdisRequest.InformationBufferLength = RequestLength - sizeof(NDIS_OID);

        /* Dispatch the request */
        NdisRequest(&Status,
                    AdapterContext->BindingHandle,
                    &NdisRequest);

        /* Wait for the request */
        if (Status == NDIS_STATUS_PENDING)
        {
            KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = AdapterContext->AsyncStatus;
        }

        /* Return the bytes read */
        if (NT_SUCCESS(Status)) Irp->IoStatus.Information = NdisRequest.BytesRead;
    }
    else
    {
        /* Bad parameters */
        Status = STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
QueryAdapterOid(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDISUIO_QUERY_OID QueryOidRequest;
    NDIS_REQUEST NdisRequest;
    ULONG RequestLength;
    NDIS_STATUS Status;

    Irp->IoStatus.Information = 0;

    QueryOidRequest = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    RequestLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    if (QueryOidRequest && RequestLength >= sizeof(NDIS_OID))
    {
        /* Setup the NDIS request */
        NdisRequest.RequestType = NdisRequestQueryInformation;
        NdisRequest.Oid = QueryOidRequest->Oid;
        NdisRequest.InformationBuffer = QueryOidRequest->Data;
        NdisRequest.InformationBufferLength = RequestLength - sizeof(NDIS_OID);
        
        /* Dispatch the request */
        NdisRequest(&Status,
                    AdapterContext->BindingHandle,
                    &NdisRequest);
        
        /* Wait for the request */
        if (Status == NDIS_STATUS_PENDING)
        {
            KeWaitForSingleObject(&AdapterContext->AsyncEvent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = AdapterContext->AsyncStatus;
        }

        /* Return the bytes written */
        if (NT_SUCCESS(Status)) Irp->IoStatus.Information = NdisRequest.BytesWritten;
    }
    else
    {
        /* Bad parameters */
        Status = STATUS_INVALID_PARAMETER;
    }
    
    Irp->IoStatus.Status = Status;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Status;
}

NTSTATUS
OpenDeviceReadWrite(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    UNICODE_STRING DeviceName;
    ULONG NameLength;
    NTSTATUS Status;
    PNDISUIO_ADAPTER_CONTEXT AdapterContext;
    PNDISUIO_OPEN_ENTRY OpenEntry;
    KIRQL OldIrql;

    NameLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    if (NameLength != 0)
    {
        DeviceName.MaximumLength = DeviceName.Length = NameLength;
        DeviceName.Buffer = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        /* Check if this already has a context */
        AdapterContext = FindAdapterContextByName(&DeviceName);
        if (AdapterContext != NULL)
        {
            /* Reference the adapter context */
            KeAcquireSpinLock(&AdapterContext->Spinlock, &OldIrql);
            ReferenceAdapterContext(AdapterContext);
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Invalid device name */
            Status = STATUS_INVALID_PARAMETER;
        }

        /* Check that the bind succeeded */
        if (NT_SUCCESS(Status))
        {
            OpenEntry = ExAllocatePool(NonPagedPool, sizeof(*OpenEntry));
            if (OpenEntry)
            {
                /* Set the file object pointer */
                OpenEntry->FileObject = FileObject;

                /* Associate this FO with the adapter */
                FileObject->FsContext = AdapterContext;
                FileObject->FsContext2 = OpenEntry;

                /* Add it to the adapter's list */
                InsertTailList(&AdapterContext->OpenEntryList,
                               &OpenEntry->ListEntry);

                /* Success */
                KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* Remove the reference we added */
                KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);
                DereferenceAdapterContext(AdapterContext, NULL);
                Status = STATUS_NO_MEMORY;
            }
        }
    }
    else
    {
        /* Invalid device name */
        Status = STATUS_INVALID_PARAMETER;
    }
    
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Status;
}

NTSTATUS
OpenDeviceWrite(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    /* FIXME: Handle this correctly */
    return OpenDeviceReadWrite(Irp, IrpSp);
}

NTSTATUS
NTAPI
NduDispatchDeviceControl(PDEVICE_OBJECT DeviceObject,
                         PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    
    ASSERT(DeviceObject == GlobalDeviceObject);

    /* Handle open IOCTLs first */
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_NDISUIO_OPEN_DEVICE:
            return OpenDeviceReadWrite(Irp, IrpSp);

        case IOCTL_NDISUIO_OPEN_WRITE_DEVICE:
            return OpenDeviceWrite(Irp, IrpSp);

        default:
            /* Fail if this file object has no adapter associated */
            if (IrpSp->FileObject->FsContext == NULL)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                
                return STATUS_INVALID_PARAMETER;
            }

            /* Now handle other IOCTLs */
            switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
            {
                case IOCTL_NDISUIO_QUERY_OID_VALUE:
                    return QueryAdapterOid(Irp, IrpSp);

                case IOCTL_NDISUIO_SET_OID_VALUE:
                    return SetAdapterOid(Irp, IrpSp);

                default:
                    DPRINT1("Unimplemented\n");
                    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                    Irp->IoStatus.Information = 0;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    break;
            }
            break;
    }
}
