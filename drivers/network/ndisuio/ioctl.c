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
WaitForBind(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    /* I've seen several code samples that use this IOCTL but there's
     * no official documentation on it. I'm just implementing it as a no-op
     * right now because I don't see any reason we need it. We handle an open
     * and bind just fine with IRP_MJ_CREATE and IOCTL_NDISUIO_OPEN_DEVICE */
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

NTSTATUS
QueryBinding(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext;
    PNDISUIO_QUERY_BINDING QueryBinding = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    ULONG BindingLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    NTSTATUS Status;
    PLIST_ENTRY CurrentEntry;
    KIRQL OldIrql;
    ULONG i;
    ULONG BytesCopied = 0;
    
    if (QueryBinding && BindingLength >= sizeof(NDISUIO_QUERY_BINDING))
    {
        KeAcquireSpinLock(&GlobalAdapterListLock, &OldIrql);
        i = 0;
        CurrentEntry = GlobalAdapterList.Flink;
        while (CurrentEntry != &GlobalAdapterList)
        {
            if (i == QueryBinding->BindingIndex)
                break;
            i++;
            CurrentEntry = CurrentEntry->Flink;
        }
        KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);
        if (i == QueryBinding->BindingIndex)
        {
            AdapterContext = CONTAINING_RECORD(CurrentEntry, NDISUIO_ADAPTER_CONTEXT, ListEntry);
            if (AdapterContext->DeviceName.Length <= QueryBinding->DeviceNameLength)
            {
                BytesCopied += AdapterContext->DeviceName.Length;
                RtlCopyMemory((PUCHAR)QueryBinding + QueryBinding->DeviceNameOffset,
                              AdapterContext->DeviceName.Buffer,
                              BytesCopied);
                QueryBinding->DeviceNameLength = AdapterContext->DeviceName.Length;

                /* FIXME: Copy description too */
                QueryBinding->DeviceDescrLength = 0;
                
                /* Successful */
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* Not enough buffer space */
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        else
        {
            /* Invalid index */
            Status = STATUS_NO_MORE_ENTRIES;
        }
    }
    else
    {
        /* Invalid parameters */
        Status = STATUS_INVALID_PARAMETER;
    }
    
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = BytesCopied;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Status;
}

NTSTATUS
CancelPacketRead(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDISUIO_PACKET_ENTRY PacketEntry;
    NTSTATUS Status;
    
    /* Indicate a 0-byte packet on the queue so one read returns 0 */
    PacketEntry = ExAllocatePool(PagedPool, sizeof(NDISUIO_PACKET_ENTRY));
    if (PacketEntry)
    {
        PacketEntry->PacketLength = 0;
        
        ExInterlockedInsertTailList(&AdapterContext->PacketList,
                                    &PacketEntry->ListEntry,
                                    &AdapterContext->Spinlock);
        
        KeSetEvent(&AdapterContext->PacketReadEvent, IO_NO_INCREMENT, FALSE);
        
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }
    
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Status;
}

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
            if (AdapterContext->OpenCount != 0)
            {
                /* An open for read-write is exclusive,
                 * so we can't have any other open handles */
                KeReleaseSpinLock(&AdapterContext->Spinlock, OldIrql);
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                /* Add a reference */
                ReferenceAdapterContext(AdapterContext);
                Status = STATUS_SUCCESS;
            }
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
                
                /* Set the permissions */
                OpenEntry->WriteOnly = FALSE;

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
                
                /* Set permissions */
                OpenEntry->WriteOnly = TRUE;
                
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
NTAPI
NduDispatchDeviceControl(PDEVICE_OBJECT DeviceObject,
                         PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PNDISUIO_OPEN_ENTRY OpenEntry;
    
    ASSERT(DeviceObject == GlobalDeviceObject);

    /* Handle open IOCTLs first */
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_NDISUIO_OPEN_DEVICE:
            return OpenDeviceReadWrite(Irp, IrpSp);

        case IOCTL_NDISUIO_OPEN_WRITE_DEVICE:
            return OpenDeviceWrite(Irp, IrpSp);
            
        case IOCTL_NDISUIO_BIND_WAIT:
            return WaitForBind(Irp, IrpSp);
            
        case IOCTL_NDISUIO_QUERY_BINDING:
            return QueryBinding(Irp, IrpSp);

        default:
            /* Fail if this file object has no adapter associated */
            if (IrpSp->FileObject->FsContext == NULL)
            {
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                
                return STATUS_INVALID_PARAMETER;
            }

            /* Now handle write IOCTLs */
            switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
            {
                case IOCTL_NDISUIO_SET_OID_VALUE:
                    return SetAdapterOid(Irp, IrpSp);

                default:
                    /* Check that we have read permissions */
                    OpenEntry = IrpSp->FileObject->FsContext2;
                    if (OpenEntry->WriteOnly)
                    {
                        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                        Irp->IoStatus.Information = 0;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        
                        return STATUS_INVALID_PARAMETER;
                    }

                    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
                    {
                        case IOCTL_CANCEL_READ:
                            return CancelPacketRead(Irp, IrpSp);
                        
                        case IOCTL_NDISUIO_QUERY_OID_VALUE:
                            return QueryAdapterOid(Irp, IrpSp);
                        
                        default:
                            DPRINT1("Unimplemented\n");
                            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                            Irp->IoStatus.Information = 0;
                            IoCompleteRequest(Irp, IO_NO_INCREMENT);
                            break;
                    }
            }
            break;
    }
}
