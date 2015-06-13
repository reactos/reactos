/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/tdi.c
 * PURPOSE:     Transport Driver Interface functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */

#include <afd.h>

#include <tdikrnl.h>
#include <tdiinfo.h>

#if DBG
#if 0
static VOID DisplayBuffer(
    PVOID Buffer,
    ULONG Size)
{
    ULONG i;
    PCHAR p;

    if ((DebugTraceLevel & MAX_TRACE) == 0)
        return;

    if (!Buffer) {
        AFD_DbgPrint(MIN_TRACE, ("Cannot display null buffer.\n"));
        return;
    }

    AFD_DbgPrint(MID_TRACE, ("Displaying buffer at (0x%X)  Size (%d).\n", Buffer, Size));

    p = (PCHAR)Buffer;
    for (i = 0; i < Size; i++) {
        if (i % 16 == 0)
            DbgPrint("\n");
        DbgPrint("%02X ", (p[i]) & 0xFF);
    }
    DbgPrint("\n");
}
#endif
#endif /* DBG */

static NTSTATUS TdiCall(
    PIRP Irp,
    PDEVICE_OBJECT DeviceObject,
    PKEVENT Event,
    PIO_STATUS_BLOCK Iosb)
/*
 * FUNCTION: Calls a transport driver device
 * ARGUMENTS:
 *     Irp           = Pointer to I/O Request Packet
 *     DeviceObject  = Pointer to device object to call
 *     Event         = An optional pointer to an event handle that will be
 *                     waited upon
 *     Iosb          = Pointer to an IO status block
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;

    AFD_DbgPrint(MID_TRACE, ("Called\n"));

    AFD_DbgPrint(MID_TRACE, ("Irp->UserEvent = %p\n", Irp->UserEvent));

    Status = IoCallDriver(DeviceObject, Irp);
    AFD_DbgPrint(MID_TRACE, ("IoCallDriver: %08x\n", Status));

    if ((Status == STATUS_PENDING) && (Event != NULL)) {
        AFD_DbgPrint(MAX_TRACE, ("Waiting on transport.\n"));
        KeWaitForSingleObject(Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = Iosb->Status;
    }

    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));

    return Status;
}


static NTSTATUS TdiOpenDevice(
    PUNICODE_STRING DeviceName,
    ULONG EaLength,
    PFILE_FULL_EA_INFORMATION EaInfo,
    ULONG ShareType,
    PHANDLE Handle,
    PFILE_OBJECT *Object)
/*
 * FUNCTION: Opens a device
 * ARGUMENTS:
 *     DeviceName = Pointer to counted string with name of device
 *     EaLength   = Length of EA information
 *     EaInfo     = Pointer to buffer with EA information
 *     Handle     = Address of buffer to place device handle
 *     Object     = Address of buffer to place device object
 * RETURNS:
 *     Status of operation
 */
{
    OBJECT_ATTRIBUTES Attr;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    ULONG ShareAccess;

    AFD_DbgPrint(MAX_TRACE, ("Called. DeviceName (%wZ, %u)\n", DeviceName, ShareType));

    /* Determine the share access */
    if (ShareType != AFD_SHARE_REUSE)
    {
        /* Exclusive access */
        ShareAccess = 0;
    }
    else
    {
        /* Shared access */
        ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }

    InitializeObjectAttributes(&Attr,                   /* Attribute buffer */
                               DeviceName,              /* Device name */
                               OBJ_CASE_INSENSITIVE |   /* Attributes */
                               OBJ_KERNEL_HANDLE,
                               NULL,                    /* Root directory */
                               NULL);                   /* Security descriptor */

    Status = ZwCreateFile(Handle,                               /* Return file handle */
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,         /* Desired access */
                          &Attr,                                /* Object attributes */
                          &Iosb,                                /* IO status */
                          0,                                    /* Initial allocation size */
                          FILE_ATTRIBUTE_NORMAL,                /* File attributes */
                          ShareAccess,                          /* Share access */
                          FILE_OPEN_IF,                         /* Create disposition */
                          0,                                    /* Create options */
                          EaInfo,                               /* EA buffer */
                          EaLength);                            /* EA length */
    if (NT_SUCCESS(Status)) {
        Status = ObReferenceObjectByHandle(*Handle,                       /* Handle to open file */
                                           GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,  /* Access mode */
                                           *IoFileObjectType,              /* Object type */
                                           KernelMode,                    /* Access mode */
                                           (PVOID*)Object,                /* Pointer to object */
                                           NULL);                         /* Handle information */
        if (!NT_SUCCESS(Status)) {
            AFD_DbgPrint(MIN_TRACE, ("ObReferenceObjectByHandle() failed with status (0x%X).\n", Status));
            ZwClose(*Handle);
        } else {
            AFD_DbgPrint(MAX_TRACE, ("Got handle (%p)  Object (%p)\n",
                                     *Handle, *Object));
        }
    } else {
        AFD_DbgPrint(MIN_TRACE, ("ZwCreateFile() failed with status (0x%X)\n", Status));
    }

    if (!NT_SUCCESS(Status)) {
        *Handle = INVALID_HANDLE_VALUE;
        *Object = NULL;
    }

    return Status;
}

NTSTATUS TdiOpenAddressFile(
    PUNICODE_STRING DeviceName,
    PTRANSPORT_ADDRESS Name,
    ULONG ShareType,
    PHANDLE AddressHandle,
    PFILE_OBJECT *AddressObject)
/*
 * FUNCTION: Opens an IPv4 address file object
 * ARGUMENTS:
 *     DeviceName    = Pointer to counted string with name of device
 *     Name          = Pointer to socket name (IPv4 address family)
 *     AddressHandle = Address of buffer to place address file handle
 *     AddressObject = Address of buffer to place address file object
 * RETURNS:
 *     Status of operation
 */
{
    PFILE_FULL_EA_INFORMATION EaInfo;
    NTSTATUS Status;
    ULONG EaLength;
    PTRANSPORT_ADDRESS Address;

    AFD_DbgPrint(MAX_TRACE, ("Called. DeviceName (%wZ)  Name (%p)\n",
                             DeviceName, Name));

    /* EaName must be 0-terminated, even though TDI_TRANSPORT_ADDRESS_LENGTH does *not* include the 0 */
    EaLength = sizeof(FILE_FULL_EA_INFORMATION) +
        TDI_TRANSPORT_ADDRESS_LENGTH +
        TaLengthOfTransportAddress( Name ) + 1;
    EaInfo = (PFILE_FULL_EA_INFORMATION)ExAllocatePool(NonPagedPool, EaLength);
    if (!EaInfo)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(EaInfo, EaLength);
    EaInfo->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    /* Don't copy the terminating 0; we have already zeroed it */
    RtlCopyMemory(EaInfo->EaName,
                  TdiTransportAddress,
                  TDI_TRANSPORT_ADDRESS_LENGTH);
    EaInfo->EaValueLength = sizeof(TA_IP_ADDRESS);
    Address =
        (PTRANSPORT_ADDRESS)(EaInfo->EaName + TDI_TRANSPORT_ADDRESS_LENGTH + 1); /* 0-terminated */
    TaCopyTransportAddressInPlace( Address, Name );

    Status = TdiOpenDevice(DeviceName,
                           EaLength,
                           EaInfo,
                           ShareType,
                           AddressHandle,
                           AddressObject);
    ExFreePool(EaInfo);
    return Status;
}

NTSTATUS TdiQueryMaxDatagramLength(
    PFILE_OBJECT FileObject,
    PUINT MaxDatagramLength)
{
    PMDL Mdl;
    PTDI_MAX_DATAGRAM_INFO Buffer;
    NTSTATUS Status = STATUS_SUCCESS;

    Buffer = ExAllocatePool(NonPagedPool, sizeof(TDI_MAX_DATAGRAM_INFO));
    if (!Buffer) return STATUS_NO_MEMORY;

    Mdl = IoAllocateMdl(Buffer, sizeof(TDI_MAX_DATAGRAM_INFO), FALSE, FALSE, NULL);
    if (!Mdl)
    {
        ExFreePool(Buffer);
        return STATUS_NO_MEMORY;
    }

    _SEH2_TRY
    {
         MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
         Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        AFD_DbgPrint(MIN_TRACE,("Failed to lock pages\n"));
        IoFreeMdl(Mdl);
        ExFreePool(Buffer);
        return Status;
    }

    Status = TdiQueryInformation(FileObject,
                                 TDI_QUERY_MAX_DATAGRAM_INFO,
                                 Mdl);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(Buffer);
        return Status;
    }

    *MaxDatagramLength = Buffer->MaxDatagramSize;

    ExFreePool(Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS TdiOpenConnectionEndpointFile(
    PUNICODE_STRING DeviceName,
    PHANDLE ConnectionHandle,
    PFILE_OBJECT *ConnectionObject)
/*
 * FUNCTION: Opens a connection endpoint file object
 * ARGUMENTS:
 *     DeviceName       = Pointer to counted string with name of device
 *     ConnectionHandle = Address of buffer to place connection endpoint file handle
 *     ConnectionObject = Address of buffer to place connection endpoint file object
 * RETURNS:
 *     Status of operation
 */
{
    PFILE_FULL_EA_INFORMATION EaInfo;
    PVOID *ContextArea;
    NTSTATUS Status;
    ULONG EaLength;

    AFD_DbgPrint(MAX_TRACE, ("Called. DeviceName (%wZ)\n", DeviceName));

    /* EaName must be 0-terminated, even though TDI_TRANSPORT_ADDRESS_LENGTH does *not* include the 0 */
    EaLength = sizeof(FILE_FULL_EA_INFORMATION) +
        TDI_CONNECTION_CONTEXT_LENGTH +
        sizeof(PVOID) + 1;

    EaInfo = (PFILE_FULL_EA_INFORMATION)ExAllocatePool(NonPagedPool, EaLength);
    if (!EaInfo)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(EaInfo, EaLength);
    EaInfo->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    /* Don't copy the terminating 0; we have already zeroed it */
    RtlCopyMemory(EaInfo->EaName,
                  TdiConnectionContext,
                  TDI_CONNECTION_CONTEXT_LENGTH);
    EaInfo->EaValueLength = sizeof(PVOID);
    ContextArea = (PVOID*)(EaInfo->EaName + TDI_CONNECTION_CONTEXT_LENGTH + 1); /* 0-terminated */
    /* FIXME: Allocate context area */
    *ContextArea = NULL;
    Status = TdiOpenDevice(DeviceName,
                           EaLength,
                           EaInfo,
                           AFD_SHARE_UNIQUE,
                           ConnectionHandle,
                           ConnectionObject);
    ExFreePool(EaInfo);
    return Status;
}


NTSTATUS TdiConnect(
    PIRP *Irp,
    PFILE_OBJECT ConnectionObject,
    PTDI_CONNECTION_INFORMATION ConnectionCallInfo,
    PTDI_CONNECTION_INFORMATION ConnectionReturnInfo,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext)
/*
 * FUNCTION: Connect a connection endpoint to a remote peer
 * ARGUMENTS:
 *     ConnectionObject = Pointer to connection endpoint file object
 *     RemoteAddress    = Pointer to remote address
 * RETURNS:
 *     Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;

    AFD_DbgPrint(MAX_TRACE, ("Called\n"));

    ASSERT(*Irp == NULL);

    if (!ConnectionObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad connection object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_CONNECT,             /* Sub function */
                                            DeviceObject,            /* Device object */
                                            ConnectionObject,        /* File object */
                                            NULL,                    /* Event */
                                            NULL);                   /* Status */
    if (!*Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildConnect(*Irp,                   /* IRP */
                    DeviceObject,           /* Device object */
                    ConnectionObject,       /* File object */
                    CompletionRoutine,      /* Completion routine */
                    CompletionContext,      /* Completion routine context */
                    NULL,                   /* Time */
                    ConnectionCallInfo,     /* Request connection information */
                    ConnectionReturnInfo);  /* Return connection information */

    TdiCall(*Irp, DeviceObject, NULL, NULL);

    return STATUS_PENDING;
}


NTSTATUS TdiAssociateAddressFile(
    HANDLE AddressHandle,
    PFILE_OBJECT ConnectionObject)
/*
 * FUNCTION: Associates a connection endpoint to an address file object
 * ARGUMENTS:
 *     AddressHandle    = Handle to address file object
 *     ConnectionObject = Connection endpoint file object
 * RETURNS:
 *     Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;

    AFD_DbgPrint(MAX_TRACE, ("Called. AddressHandle (%p)  ConnectionObject (%p)\n",
                             AddressHandle, ConnectionObject));

    if (!ConnectionObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad connection object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_ASSOCIATE_ADDRESS,   /* Sub function */
                                           DeviceObject,            /* Device object */
                                           ConnectionObject,        /* File object */
                                           &Event,                  /* Event */
                                           &Iosb);                  /* Status */
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    TdiBuildAssociateAddress(Irp,
                             DeviceObject,
                             ConnectionObject,
                             NULL,
                             NULL,
                             AddressHandle);

    return TdiCall(Irp, DeviceObject, &Event, &Iosb);
}

NTSTATUS TdiDisassociateAddressFile(
    PFILE_OBJECT ConnectionObject)
/*
 * FUNCTION: Disassociates a connection endpoint from an address file object
 * ARGUMENTS:
 *     ConnectionObject = Connection endpoint file object
 * RETURNS:
 *     Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;

    AFD_DbgPrint(MAX_TRACE, ("Called. ConnectionObject (%p)\n", ConnectionObject));

    if (!ConnectionObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad connection object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_DISASSOCIATE_ADDRESS,   /* Sub function */
                                           DeviceObject,            /* Device object */
                                           ConnectionObject,        /* File object */
                                           &Event,                  /* Event */
                                           &Iosb);                  /* Status */
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    TdiBuildDisassociateAddress(Irp,
                                DeviceObject,
                                ConnectionObject,
                                NULL,
                                NULL);

    return TdiCall(Irp, DeviceObject, &Event, &Iosb);
}

NTSTATUS TdiListen(
    PIRP *Irp,
    PFILE_OBJECT ConnectionObject,
    PTDI_CONNECTION_INFORMATION *RequestConnectionInfo,
    PTDI_CONNECTION_INFORMATION *ReturnConnectionInfo,
    PIO_COMPLETION_ROUTINE  CompletionRoutine,
    PVOID CompletionContext)
/*
 * FUNCTION: Listen on a connection endpoint for a connection request from a remote peer
 * ARGUMENTS:
 *     CompletionRoutine = Routine to be called when IRP is completed
 *     CompletionContext = Context for CompletionRoutine
 * RETURNS:
 *     Status of operation
 *     May return STATUS_PENDING
 */
{
    PDEVICE_OBJECT DeviceObject;

    AFD_DbgPrint(MAX_TRACE, ("Called\n"));

    ASSERT(*Irp == NULL);

    if (!ConnectionObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad connection object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_LISTEN,              /* Sub function */
                                            DeviceObject,            /* Device object */
                                            ConnectionObject,        /* File object */
                                            NULL,                    /* Event */
                                            NULL);                   /* Status */
    if (*Irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    TdiBuildListen(*Irp,                   /* IRP */
                   DeviceObject,           /* Device object */
                   ConnectionObject,       /* File object */
                   CompletionRoutine,      /* Completion routine */
                   CompletionContext,      /* Completion routine context */
                   0,                      /* Flags */
                   *RequestConnectionInfo, /* Request connection information */
                   *ReturnConnectionInfo);  /* Return connection information */

    TdiCall(*Irp, DeviceObject, NULL /* Don't wait for completion */, NULL);

    return STATUS_PENDING;
}


NTSTATUS TdiSetEventHandler(
    PFILE_OBJECT FileObject,
    LONG EventType,
    PVOID Handler,
    PVOID Context)
/*
 * FUNCTION: Sets or resets an event handler
 * ARGUMENTS:
 *     FileObject = Pointer to file object
 *     EventType  = Event code
 *     Handler    = Event handler to be called when the event occurs
 *     Context    = Context input to handler when the event occurs
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Specify NULL for Handler to stop calling event handler
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;

    AFD_DbgPrint(MAX_TRACE, ("Called\n"));

    if (!FileObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad file object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_SET_EVENT_HANDLER,   /* Sub function */
                                           DeviceObject,            /* Device object */
                                           FileObject,              /* File object */
                                           &Event,                  /* Event */
                                           &Iosb);                  /* Status */
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;



    TdiBuildSetEventHandler(Irp,
                            DeviceObject,
                            FileObject,
                            NULL,
                            NULL,
                            EventType,
                            Handler,
                            Context);

    return TdiCall(Irp, DeviceObject, &Event, &Iosb);
}


NTSTATUS TdiQueryDeviceControl(
    PFILE_OBJECT FileObject,
    ULONG IoControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG Return)
/*
 * FUNCTION: Queries a device for information
 * ARGUMENTS:
 *     FileObject         = Pointer to file object
 *     IoControlCode      = I/O control code
 *     InputBuffer        = Pointer to buffer with input data
 *     InputBufferLength  = Length of InputBuffer
 *     OutputBuffer       = Address of buffer to place output data
 *     OutputBufferLength = Length of OutputBuffer
 * RETURNS:
 *     Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    KEVENT Event;
    PIRP Irp;

    if (!FileObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad file object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE,
                                        &Event,
                                        &Iosb);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = TdiCall(Irp, DeviceObject, &Event, &Iosb);

    if (Return)
        *Return = Iosb.Information;

    return Status;
}


NTSTATUS TdiQueryInformation(
    PFILE_OBJECT FileObject,
    LONG QueryType,
    PMDL MdlBuffer)
/*
 * FUNCTION: Query for information
 * ARGUMENTS:
 *     FileObject   = Pointer to file object
 *     QueryType    = Query type
 *     MdlBuffer    = Pointer to MDL buffer specific for query type
 * RETURNS:
 *     Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;

    if (!FileObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad file object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_QUERY_INFORMATION,       /* Sub function */
                                           DeviceObject,                /* Device object */
                                           ConnectionObject,            /* File object */
                                           &Event,                      /* Event */
                                           &Iosb);                      /* Status */
    if (!Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildQueryInformation(Irp,
                             DeviceObject,
                             FileObject,
                             NULL,
                             NULL,
                             QueryType,
                             MdlBuffer);

    return TdiCall(Irp, DeviceObject, &Event, &Iosb);
}

NTSTATUS TdiQueryInformationEx(
    PFILE_OBJECT FileObject,
    ULONG Entity,
    ULONG Instance,
    ULONG Class,
    ULONG Type,
    ULONG Id,
    PVOID OutputBuffer,
    PULONG OutputLength)
/*
 * FUNCTION: Extended query for information
 * ARGUMENTS:
 *     FileObject   = Pointer to file object
 *     Entity       = Entity
 *     Instance     = Instance
 *     Class        = Entity class
 *     Type         = Entity type
 *     Id           = Entity id
 *     OutputBuffer = Address of buffer to place data
 *     OutputLength = Address of buffer with length of OutputBuffer (updated)
 * RETURNS:
 *     Status of operation
 */
{
    TCP_REQUEST_QUERY_INFORMATION_EX QueryInfo;

    RtlZeroMemory(&QueryInfo, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
    QueryInfo.ID.toi_entity.tei_entity   = Entity;
    QueryInfo.ID.toi_entity.tei_instance = Instance;
    QueryInfo.ID.toi_class = Class;
    QueryInfo.ID.toi_type  = Type;
    QueryInfo.ID.toi_id    = Id;

    return TdiQueryDeviceControl(FileObject,                                /* Transport/connection object */
                                 IOCTL_TCP_QUERY_INFORMATION_EX,            /* Control code */
                                 &QueryInfo,                                /* Input buffer */
                                 sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),  /* Input buffer length */
                                 OutputBuffer,                              /* Output buffer */
                                 *OutputLength,                             /* Output buffer length */
                                 OutputLength);                             /* Return information */
}

NTSTATUS TdiQueryAddress(
    PFILE_OBJECT FileObject,
    PULONG Address)
/*
 * FUNCTION: Queries for a local IP address
 * ARGUMENTS:
 *     FileObject = Pointer to file object
 *     Address    = Address of buffer to place local address
 * RETURNS:
 *     Status of operation
 */
{
    UINT i;
    TDIEntityID *Entities;
    ULONG EntityCount;
    ULONG EntityType;
    IPSNMPInfo SnmpInfo;
    PIPADDR_ENTRY IpAddress;
    ULONG BufferSize;
    NTSTATUS Status = STATUS_SUCCESS;

    AFD_DbgPrint(MAX_TRACE, ("Called\n"));

    BufferSize = sizeof(TDIEntityID) * 20;
    Entities   = (TDIEntityID*)ExAllocatePool(NonPagedPool, BufferSize);
    if (!Entities) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Query device for supported entities */

    Status = TdiQueryInformationEx(FileObject,          /* File object */
                                   GENERIC_ENTITY,      /* Entity */
                                   TL_INSTANCE,         /* Instance */
                                   INFO_CLASS_GENERIC,  /* Entity class */
                                   INFO_TYPE_PROVIDER,  /* Entity type */
                                   ENTITY_LIST_ID,      /* Entity id */
                                   Entities,            /* Output buffer */
                                   &BufferSize);        /* Output buffer size */
    if (!NT_SUCCESS(Status)) {
        AFD_DbgPrint(MIN_TRACE, ("Unable to get list of supported entities (Status = 0x%X).\n", Status));
        ExFreePool(Entities);
        return Status;
    }

    /* Locate an IP entity */
    EntityCount = BufferSize / sizeof(TDIEntityID);

    AFD_DbgPrint(MAX_TRACE, ("EntityCount = %u\n", EntityCount));

    for (i = 0; i < EntityCount; i++) {
        if (Entities[i].tei_entity == CL_NL_ENTITY) {
            /* Query device for entity type */

            BufferSize = sizeof(EntityType);
            Status = TdiQueryInformationEx(FileObject,                  /* File object */
                                           CL_NL_ENTITY,                /* Entity */
                                           Entities[i].tei_instance,    /* Instance */
                                           INFO_CLASS_GENERIC,          /* Entity class */
                                           INFO_TYPE_PROVIDER,          /* Entity type */
                                           ENTITY_TYPE_ID,              /* Entity id */
                                           &EntityType,                 /* Output buffer */
                                           &BufferSize);                /* Output buffer size */
            if (!NT_SUCCESS(Status) || (EntityType != CL_NL_IP)) {
                AFD_DbgPrint(MIN_TRACE, ("Unable to get entity of type IP (Status = 0x%X).\n", Status));
                break;
            }

            /* Query device for SNMP information */

            BufferSize = sizeof(SnmpInfo);
            Status = TdiQueryInformationEx(FileObject,                  /* File object */
                                           CL_NL_ENTITY,                /* Entity */
                                           Entities[i].tei_instance,    /* Instance */
                                           INFO_CLASS_PROTOCOL,         /* Entity class */
                                           INFO_TYPE_PROVIDER,          /* Entity type */
                                           IP_MIB_STATS_ID,             /* Entity id */
                                           &SnmpInfo,                   /* Output buffer */
                                           &BufferSize);                /* Output buffer size */
            if (!NT_SUCCESS(Status) || (SnmpInfo.ipsi_numaddr == 0)) {
                AFD_DbgPrint(MIN_TRACE, ("Unable to get SNMP information or no IP addresses available (Status = 0x%X).\n", Status));
                break;
            }

            /* Query device for all IP addresses */

            if (SnmpInfo.ipsi_numaddr != 0) {
                BufferSize = SnmpInfo.ipsi_numaddr * sizeof(IPADDR_ENTRY);
                IpAddress = (PIPADDR_ENTRY)ExAllocatePool(NonPagedPool, BufferSize);
                if (!IpAddress) {
                    AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
                    break;
                }

                Status = TdiQueryInformationEx(FileObject,                  /* File object */
                                               CL_NL_ENTITY,                /* Entity */
                                               Entities[i].tei_instance,    /* Instance */
                                               INFO_CLASS_PROTOCOL,         /* Entity class */
                                               INFO_TYPE_PROVIDER,          /* Entity type */
                                               IP_MIB_ADDRTABLE_ENTRY_ID,   /* Entity id */
                                               IpAddress,                   /* Output buffer */
                                               &BufferSize);                /* Output buffer size */
                if (!NT_SUCCESS(Status)) {
                    AFD_DbgPrint(MIN_TRACE, ("Unable to get IP address (Status = 0x%X).\n", Status));
                    ExFreePool(IpAddress);
                    break;
                }

                if (SnmpInfo.ipsi_numaddr != 1) {
                    /* Skip loopback address */
                    *Address = DN2H(IpAddress[1].Addr);
                } else {
                    /* Select the first address returned */
                    *Address = DN2H(IpAddress->Addr);
                }

                ExFreePool(IpAddress);
            } else {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        }
    }

    ExFreePool(Entities);

    AFD_DbgPrint(MAX_TRACE, ("Leaving\n"));

    return Status;
}

NTSTATUS TdiSend(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext)
{
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl;

    ASSERT(*Irp == NULL);

    if (!TransportObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad transport object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_SEND,                /* Sub function */
                                            DeviceObject,            /* Device object */
                                            TransportObject,         /* File object */
                                            NULL,                    /* Event */
                                            NULL);                   /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE, ("Allocating irp for %p:%u\n", Buffer,BufferLength));

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        MmProbeAndLockPages(Mdl, (*Irp)->RequestorMode, IoReadAccess);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
        IoFreeMdl(Mdl);
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH2_END;

    AFD_DbgPrint(MID_TRACE,("AFD>>> Got an MDL: %p\n", Mdl));

    TdiBuildSend(*Irp,                   /* I/O Request Packet */
                 DeviceObject,           /* Device object */
                 TransportObject,        /* File object */
                 CompletionRoutine,      /* Completion routine */
                 CompletionContext,      /* Completion context */
                 Mdl,                    /* Data buffer */
                 Flags,                  /* Flags */
                 BufferLength);          /* Length of data */

    TdiCall(*Irp, DeviceObject, NULL, NULL);
    /* Does not block...  The MDL is deleted in the receive completion
       routine. */

    return STATUS_PENDING;
}

NTSTATUS TdiReceive(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext)
{
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl;

    ASSERT(*Irp == NULL);

    if (!TransportObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad transport object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_RECEIVE,             /* Sub function */
                                            DeviceObject,            /* Device object */
                                            TransportObject,         /* File object */
                                            NULL,                    /* Event */
                                            NULL);                   /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE, ("Allocating irp for %p:%u\n", Buffer,BufferLength));

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        AFD_DbgPrint(MID_TRACE, ("probe and lock\n"));
        MmProbeAndLockPages(Mdl, (*Irp)->RequestorMode, IoModifyAccess);
        AFD_DbgPrint(MID_TRACE, ("probe and lock done\n"));
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
        IoFreeMdl(Mdl);
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH2_END;

    AFD_DbgPrint(MID_TRACE,("AFD>>> Got an MDL: %p\n", Mdl));

    TdiBuildReceive(*Irp,                   /* I/O Request Packet */
                    DeviceObject,           /* Device object */
                    TransportObject,        /* File object */
                    CompletionRoutine,      /* Completion routine */
                    CompletionContext,      /* Completion context */
                    Mdl,                    /* Data buffer */
                    Flags,                  /* Flags */
                    BufferLength);          /* Length of data */


    TdiCall(*Irp, DeviceObject, NULL, NULL);
    /* Does not block...  The MDL is deleted in the receive completion
       routine. */

    return STATUS_PENDING;
}


NTSTATUS TdiReceiveDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION Addr,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext)
/*
 * FUNCTION: Receives a datagram
 * ARGUMENTS:
 *     TransportObject = Pointer to transport object
 *     From            = Receive filter (NULL if none)
 *     Address         = Address of buffer to place remote address
 *     Buffer          = Address of buffer to place received data
 *     BufferSize      = Address of buffer with length of Buffer (updated)
 * RETURNS:
 *     Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl;

    ASSERT(*Irp == NULL);

    if (!TransportObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad tranport object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_RECEIVE_DATAGRAM,    /* Sub function */
                                            DeviceObject,            /* Device object */
                                            TransportObject,         /* File object */
                                            NULL,                    /* Event */
                                            NULL);                   /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE, ("Allocating irp for %p:%u\n", Buffer,BufferLength));

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        MmProbeAndLockPages(Mdl, (*Irp)->RequestorMode, IoModifyAccess);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
        IoFreeMdl(Mdl);
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH2_END;

    AFD_DbgPrint(MID_TRACE,("AFD>>> Got an MDL: %p\n", Mdl));

    TdiBuildReceiveDatagram(*Irp,                   /* I/O Request Packet */
                            DeviceObject,           /* Device object */
                            TransportObject,        /* File object */
                            CompletionRoutine,      /* Completion routine */
                            CompletionContext,      /* Completion context */
                            Mdl,                    /* Data buffer */
                            BufferLength,
                            Addr,
                            Addr,
                            Flags);                 /* Length of data */

    TdiCall(*Irp, DeviceObject, NULL, NULL);
    /* Does not block...  The MDL is deleted in the receive completion
       routine. */

    return STATUS_PENDING;
}


NTSTATUS TdiSendDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION Addr,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext)
/*
 * FUNCTION: Sends a datagram
 * ARGUMENTS:
 *     TransportObject = Pointer to transport object
 *     From            = Send filter (NULL if none)
 *     Address         = Address of buffer to place remote address
 *     Buffer          = Address of buffer to place send data
 *     BufferSize      = Address of buffer with length of Buffer (updated)
 * RETURNS:
 *     Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl;

    ASSERT(*Irp == NULL);

    if (!TransportObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad transport object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    AFD_DbgPrint(MID_TRACE,("Called(TransportObject %p)\n", TransportObject));

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_SEND_DATAGRAM,       /* Sub function */
                                            DeviceObject,            /* Device object */
                                            TransportObject,         /* File object */
                                            NULL,                    /* Event */
                                            NULL);                   /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE, ("Allocating irp for %p:%u\n", Buffer,BufferLength));

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */

    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        MmProbeAndLockPages(Mdl, (*Irp)->RequestorMode, IoReadAccess);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
        IoFreeMdl(Mdl);
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH2_END;

    AFD_DbgPrint(MID_TRACE,("AFD>>> Got an MDL: %p\n", Mdl));

    TdiBuildSendDatagram(*Irp,                   /* I/O Request Packet */
                         DeviceObject,           /* Device object */
                         TransportObject,        /* File object */
                         CompletionRoutine,      /* Completion routine */
                         CompletionContext,      /* Completion context */
                         Mdl,                    /* Data buffer */
                         BufferLength,           /* Bytes to send */
                         Addr);                  /* Address */

    TdiCall(*Irp, DeviceObject, NULL, NULL);
    /* Does not block...  The MDL is deleted in the send completion
       routine. */

    return STATUS_PENDING;
}

NTSTATUS TdiDisconnect(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    PLARGE_INTEGER Time,
    USHORT Flags,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext,
    PTDI_CONNECTION_INFORMATION RequestConnectionInfo,
    PTDI_CONNECTION_INFORMATION ReturnConnectionInfo) {
    PDEVICE_OBJECT DeviceObject;

    if (!TransportObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad transport object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    AFD_DbgPrint(MID_TRACE,("Called(TransportObject %p)\n", TransportObject));

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_DISCONNECT,          /* Sub function */
                                            DeviceObject,            /* Device object */
                                            TransportObject,         /* File object */
                                            NULL,                    /* Event */
                                            NULL);                   /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildDisconnect(*Irp,                   /* I/O Request Packet */
                       DeviceObject,           /* Device object */
                       TransportObject,        /* File object */
                       CompletionRoutine,      /* Completion routine */
                       CompletionContext,      /* Completion context */
                       Time,                   /* Time */
                       Flags,                  /* Disconnect flags */
                       RequestConnectionInfo,  /* Indication of who to disconnect */
                       ReturnConnectionInfo);  /* Indication of who disconnected */

    TdiCall(*Irp, DeviceObject, NULL, NULL);

    return STATUS_PENDING;
}

/* EOF */
