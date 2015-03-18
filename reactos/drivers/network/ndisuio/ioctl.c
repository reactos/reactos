/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        ioctl.c
 * PURPOSE:     IOCTL handling
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "ndisuio.h"

//#define NDEBUG
#include <debug.h>

static
NTSTATUS
WaitForBind(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    /* I've seen several code samples that use this IOCTL but there's
     * no official documentation on it. I'm just implementing it as a no-op
     * right now because I don't see any reason we need it. We handle an open
     * and bind just fine with IRP_MJ_CREATE and IOCTL_NDISUIO_OPEN_DEVICE */
    DPRINT("Wait for bind complete\n");
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

static
NTSTATUS
QueryBinding(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = NULL;
    PNDISUIO_QUERY_BINDING QueryBinding = Irp->AssociatedIrp.SystemBuffer;
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
            {
                AdapterContext = CONTAINING_RECORD(CurrentEntry, NDISUIO_ADAPTER_CONTEXT, ListEntry);
                break;
            }
            i++;
            CurrentEntry = CurrentEntry->Flink;
        }
        KeReleaseSpinLock(&GlobalAdapterListLock, OldIrql);
        if (AdapterContext)
        {
            DPRINT("Query binding for index %d is adapter %wZ\n", i, &AdapterContext->DeviceName);
            BytesCopied = sizeof(NDISUIO_QUERY_BINDING);
            if (AdapterContext->DeviceName.Length <= BindingLength - BytesCopied)
            {
                QueryBinding->DeviceNameOffset = BytesCopied;
                QueryBinding->DeviceNameLength = AdapterContext->DeviceName.Length;
                RtlCopyMemory((PUCHAR)QueryBinding + QueryBinding->DeviceNameOffset,
                              AdapterContext->DeviceName.Buffer,
                              QueryBinding->DeviceNameLength);
                BytesCopied += AdapterContext->DeviceName.Length;

                /* FIXME: Copy description too */
                QueryBinding->DeviceDescrOffset = BytesCopied;
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

#if 0
static
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
        
        ExInterlockedInsertHeadList(&AdapterContext->PacketList,
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
#endif

static
NTSTATUS
SetAdapterOid(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDISUIO_SET_OID SetOidRequest;
    NDIS_REQUEST Request;
    ULONG RequestLength;
    NDIS_STATUS Status;
    
    Irp->IoStatus.Information = 0;
    
    SetOidRequest = Irp->AssociatedIrp.SystemBuffer;
    RequestLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    if (SetOidRequest && RequestLength >= sizeof(NDIS_OID))
    {
        /* Setup the NDIS request */
        Request.RequestType = NdisRequestSetInformation;
        Request.DATA.SET_INFORMATION.Oid = SetOidRequest->Oid;
        Request.DATA.SET_INFORMATION.InformationBufferLength = RequestLength - sizeof(NDIS_OID);
        if (Request.DATA.SET_INFORMATION.InformationBufferLength != 0)
        {
            Request.DATA.SET_INFORMATION.InformationBuffer = SetOidRequest->Data;
        }
        else
        {
            Request.DATA.SET_INFORMATION.InformationBuffer = NULL;
        }
        Request.DATA.SET_INFORMATION.BytesRead = 0;

        DPRINT("Setting OID 0x%x on adapter %wZ\n", SetOidRequest->Oid, &AdapterContext->DeviceName);

        /* Dispatch the request */
        NdisRequest(&Status,
                    AdapterContext->BindingHandle,
                    &Request);

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
        if (Status == NDIS_STATUS_INVALID_LENGTH ||
            Status == NDIS_STATUS_BUFFER_TOO_SHORT)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else if (Status == NDIS_STATUS_SUCCESS)
        {
            Irp->IoStatus.Information = sizeof(NDIS_OID) + Request.DATA.SET_INFORMATION.BytesRead;
        }

        DPRINT("Final request status: 0x%x (%d)\n", Status, Irp->IoStatus.Information);
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

static
NTSTATUS
QueryAdapterOid(PIRP Irp, PIO_STACK_LOCATION IrpSp)
{
    PNDISUIO_ADAPTER_CONTEXT AdapterContext = IrpSp->FileObject->FsContext;
    PNDISUIO_QUERY_OID QueryOidRequest;
    NDIS_REQUEST Request;
    ULONG RequestLength;
    NDIS_STATUS Status;

    Irp->IoStatus.Information = 0;

    QueryOidRequest = Irp->AssociatedIrp.SystemBuffer;
    RequestLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    if (QueryOidRequest && RequestLength >= sizeof(NDIS_OID))
    {
        /* Setup the NDIS request */
        Request.RequestType = NdisRequestQueryInformation;
        Request.DATA.QUERY_INFORMATION.Oid = QueryOidRequest->Oid;
        Request.DATA.QUERY_INFORMATION.InformationBufferLength = RequestLength - sizeof(NDIS_OID);
        if (Request.DATA.QUERY_INFORMATION.InformationBufferLength != 0)
        {
            Request.DATA.QUERY_INFORMATION.InformationBuffer = QueryOidRequest->Data;
        }
        else
        {
            Request.DATA.QUERY_INFORMATION.InformationBuffer = NULL;
        }
        Request.DATA.QUERY_INFORMATION.BytesWritten = 0;

        DPRINT("Querying OID 0x%x on adapter %wZ\n", QueryOidRequest->Oid, &AdapterContext->DeviceName);
        
        /* Dispatch the request */
        NdisRequest(&Status,
                    AdapterContext->BindingHandle,
                    &Request);
        
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
        if (Status == NDIS_STATUS_INVALID_LENGTH ||
            Status == NDIS_STATUS_BUFFER_TOO_SHORT)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else if (Status == NDIS_STATUS_SUCCESS)
        {
            Irp->IoStatus.Information = sizeof(NDIS_OID) + Request.DATA.QUERY_INFORMATION.BytesWritten;
        }

        DPRINT("Final request status: 0x%x (%d)\n", Status, Irp->IoStatus.Information);
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

static
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
        DeviceName.Buffer = Irp->AssociatedIrp.SystemBuffer;

        /* Check if this already has a context */
        AdapterContext = FindAdapterContextByName(&DeviceName);
        if (AdapterContext != NULL)
        {
            DPRINT("Binding file object 0x%x to device %wZ\n", FileObject, &AdapterContext->DeviceName);

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
                DereferenceAdapterContextWithOpenEntry(AdapterContext, NULL);
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

#if 0
static
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
        DeviceName.Buffer = Irp->AssociatedIrp.SystemBuffer;
        
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
#endif

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
#if 0
        case IOCTL_NDISUIO_OPEN_WRITE_DEVICE:
            return OpenDeviceWrite(Irp, IrpSp);
#endif
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
#if 0
                        case IOCTL_CANCEL_READ:
                            return CancelPacketRead(Irp, IrpSp);
#endif
                        
                        case IOCTL_NDISUIO_QUERY_OID_VALUE:
                            return QueryAdapterOid(Irp, IrpSp);
                        
                        default:
                            DPRINT1("Unimplemented\n");
                            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                            Irp->IoStatus.Information = 0;
                            IoCompleteRequest(Irp, IO_NO_INCREMENT);
                            return STATUS_NOT_IMPLEMENTED;
                    }
            }
            break;
    }
}
