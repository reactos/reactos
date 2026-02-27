/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        tdihelpers/tdi.c
 * PURPOSE:     Transport Driver Interface functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 *   20251021 Moved to tdihelpers since it is used by 2 drivers (AFD, NETIO) now
 */

#include <afd.h>

#ifdef UNIMPLEMENTED
#undef UNIMPLEMENTED
#endif

#include <tdikrnl.h>
#include <tdiinfo.h>

/* If you want to see the DPRINT() output in your debugger,
 * remove the following line (or comment it out): */
#define NDEBUG
#include <reactos/debug.h>

static NTSTATUS TdiCall(
    PIRP Irp,
    PDEVICE_OBJECT DeviceObject,
    PKEVENT Event,
    PIO_STATUS_BLOCK Iosb)
/*!
 * @brief Calls a transport driver device
 *
 * @param    Irp           = Pointer to I/O Request Packet
 * @param    DeviceObject  = Pointer to device object to call
 * @param    Event         = An optional pointer to an event handle that will be
 *                           waited upon
 * @param    Iosb          = Pointer to an IO status block
 *
 * @return   Status of operation
 */
{
    NTSTATUS Status;

    DPRINT("Called\n");

    DPRINT("Irp->UserEvent = %p\n", Irp->UserEvent);

    Status = IoCallDriver(DeviceObject, Irp);
    DPRINT("IoCallDriver: %08x\n", Status);

    if ((Status == STATUS_PENDING) && (Event != NULL)) {
        DPRINT("Waiting on transport.\n");
        KeWaitForSingleObject(Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = Iosb->Status;
    }

    DPRINT("Status (0x%X).\n", Status);

    return Status;
}


static NTSTATUS TdiOpenDevice(
    PUNICODE_STRING DeviceName,
    ULONG EaLength,
    PFILE_FULL_EA_INFORMATION EaInfo,
    ULONG ShareType,
    PHANDLE Handle,
    PFILE_OBJECT *Object)
/*!
 * @brief Opens a device
 *
 * @param    DeviceName = Pointer to counted string with name of device
 * @param    EaLength   = Length of EA information
 * @param    EaInfo     = Pointer to buffer with EA information
 * @param    Handle     = Address of buffer to place device handle
 * @param    Object     = Address of buffer to place device object
 *
 * @return   Status of operation
 */
{
    OBJECT_ATTRIBUTES Attr;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    ULONG ShareAccess;

    DPRINT("Called. DeviceName (%wZ, %u)\n", DeviceName, ShareType);

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
            DPRINT("ObReferenceObjectByHandle() failed with status (0x%X).\n", Status);
            ZwClose(*Handle);
        } else {
            DPRINT("Got handle (%p)  Object (%p)\n", *Handle, *Object);
        }
    } else {
        DPRINT("ZwCreateFile() failed with status (0x%X)\n", Status);
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
/*!
 * @brief Opens an IPv4 address file object
 *
 * @param    DeviceName    = Pointer to counted string with name of device
 * @param    Name          = Pointer to socket name (IPv4 address family)
 * @param    AddressHandle = Address of buffer to place address file handle
 * @param    AddressObject = Address of buffer to place address file object
 *
 * @return   Status of operation
 */
{
    PFILE_FULL_EA_INFORMATION EaInfo;
    NTSTATUS Status;
    ULONG EaLength;
    PTRANSPORT_ADDRESS Address;

    DPRINT("Called. DeviceName (%wZ)  Name (%p)\n", DeviceName, Name);

    /* EaName must be 0-terminated, even though TDI_TRANSPORT_ADDRESS_LENGTH does *not* include the 0 */
    EaLength = sizeof(FILE_FULL_EA_INFORMATION) +
        TDI_TRANSPORT_ADDRESS_LENGTH +
        TaLengthOfTransportAddress( Name ) + 1;
    EaInfo = (PFILE_FULL_EA_INFORMATION)ExAllocatePoolWithTag(NonPagedPool,
                                                              EaLength,
                                                              TAG_AFD_EA_INFO);
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
    ExFreePoolWithTag(EaInfo, TAG_AFD_EA_INFO);
    return Status;
}

NTSTATUS TdiQueryMaxDatagramLength(
    PFILE_OBJECT FileObject,
    PUINT MaxDatagramLength)
{
    PMDL Mdl;
    PTDI_MAX_DATAGRAM_INFO Buffer;
    NTSTATUS Status = STATUS_SUCCESS;

    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   sizeof(TDI_MAX_DATAGRAM_INFO),
                                   TAG_AFD_DATA_BUFFER);

    if (!Buffer) return STATUS_NO_MEMORY;

    Mdl = IoAllocateMdl(Buffer, sizeof(TDI_MAX_DATAGRAM_INFO), FALSE, FALSE, NULL);
    if (!Mdl)
    {
        ExFreePoolWithTag(Buffer, TAG_AFD_DATA_BUFFER);
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
        DPRINT("Failed to lock pages\n");
        IoFreeMdl(Mdl);
        ExFreePoolWithTag(Buffer, TAG_AFD_DATA_BUFFER);
        return Status;
    }

    Status = TdiQueryInformation(FileObject,
                                 TDI_QUERY_MAX_DATAGRAM_INFO,
                                 Mdl);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Buffer, TAG_AFD_DATA_BUFFER);
        return Status;
    }

    *MaxDatagramLength = Buffer->MaxDatagramSize;

    ExFreePoolWithTag(Buffer, TAG_AFD_DATA_BUFFER);

    return STATUS_SUCCESS;
}

NTSTATUS TdiOpenConnectionEndpointFile(
    PUNICODE_STRING DeviceName,
    PHANDLE ConnectionHandle,
    PFILE_OBJECT *ConnectionObject)
/*!
 * @brief Opens a connection endpoint file object
 *
 * @param    DeviceName       = Pointer to counted string with name of device
 * @param    ConnectionHandle = Address of buffer to place connection endpoint file handle
 * @param    ConnectionObject = Address of buffer to place connection endpoint file object
 *
 * @return   Status of operation
 */
{
    PFILE_FULL_EA_INFORMATION EaInfo;
    PVOID *ContextArea;
    NTSTATUS Status;
    ULONG EaLength;

    DPRINT("Called. DeviceName (%wZ)\n", DeviceName);

    /* EaName must be 0-terminated, even though TDI_TRANSPORT_ADDRESS_LENGTH does *not* include the 0 */
    EaLength = sizeof(FILE_FULL_EA_INFORMATION) +
        TDI_CONNECTION_CONTEXT_LENGTH +
        sizeof(PVOID) + 1;

    EaInfo = (PFILE_FULL_EA_INFORMATION)ExAllocatePoolWithTag(NonPagedPool,
                                                              EaLength,
                                                              TAG_AFD_EA_INFO);
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
    ExFreePoolWithTag(EaInfo, TAG_AFD_EA_INFO);
    return Status;
}


NTSTATUS TdiConnect(
    PIRP *Irp,
    PFILE_OBJECT ConnectionObject,
    PTDI_CONNECTION_INFORMATION ConnectionCallInfo,
    PTDI_CONNECTION_INFORMATION ConnectionReturnInfo,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext)
/*!
 * @brief Connect a connection endpoint to a remote peer
 *
 * @param    ConnectionObject = Pointer to connection endpoint file object
 * @param    RemoteAddress    = Pointer to remote address
 *
 * @return   Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;

    DPRINT("Called\n");

    if (!ConnectionObject) {
        DPRINT("Bad connection object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_CONNECT,
                                            DeviceObject,
                                            ConnectionObject,
                                            NULL,
                                            NULL);
    if (!*Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildConnect(*Irp,
                    DeviceObject,
                    ConnectionObject,
                    CompletionRoutine,
                    CompletionContext,
                    NULL,
                    ConnectionCallInfo,
                    ConnectionReturnInfo);

    return TdiCall(*Irp, DeviceObject, NULL, NULL);
}


NTSTATUS TdiAssociateAddressFile(
    HANDLE AddressHandle,
    PFILE_OBJECT ConnectionObject)
/*!
 * @brief Associates a connection endpoint to an address file object
 *
 * @param    AddressHandle    = Handle to address file object
 * @param    ConnectionObject = Connection endpoint file object
 *
 * @return   Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;

    DPRINT("Called. AddressHandle (%p)  ConnectionObject (%p)\n", AddressHandle, ConnectionObject);

    if (!ConnectionObject) {
        DPRINT("Bad connection object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_ASSOCIATE_ADDRESS,
                                           DeviceObject,
                                           ConnectionObject,
                                           &Event,
                                           &Iosb);
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
/*!
 * @brief Disassociates a connection endpoint from an address file object
 *
 * @param    ConnectionObject = Connection endpoint file object
 *
 * @return   Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;

    DPRINT("Called. ConnectionObject (%p)\n", ConnectionObject);

    if (!ConnectionObject) {
        DPRINT("Bad connection object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_DISASSOCIATE_ADDRESS,
                                           DeviceObject,
                                           ConnectionObject,
                                           &Event,
                                           &Iosb);
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
/*!
 * @brief    Listen on a connection endpoint for a connection request from a remote peer
 *
 * @param    CompletionRoutine = Routine to be called when IRP is completed
 * @param    CompletionContext = Context for CompletionRoutine
 *
 * @return   Status of operation
 *           May return STATUS_PENDING
 */
{
    PDEVICE_OBJECT DeviceObject;

    DPRINT("Called\n");

    if (!ConnectionObject) {
        DPRINT("Bad connection object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_LISTEN,
                                            DeviceObject,
                                            ConnectionObject,
                                            NULL,
                                            NULL);
    if (*Irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    TdiBuildListen(*Irp,
                   DeviceObject,
                   ConnectionObject,
                   CompletionRoutine,
                   CompletionContext,
                   0,
                   *RequestConnectionInfo,
                   *ReturnConnectionInfo);

    TdiCall(*Irp, DeviceObject, NULL /* Don't wait for completion */, NULL);

    return STATUS_PENDING;
}

NTSTATUS TdiAccept(
    PIRP *Irp,
    PFILE_OBJECT AcceptConnectionObject,
    PTDI_CONNECTION_INFORMATION RequestConnectionInfo,
    PTDI_CONNECTION_INFORMATION ReturnConnectionInfo,
    PIO_COMPLETION_ROUTINE  CompletionRoutine,
    PVOID CompletionContext)
/*!
 * @brief Listen on a connection endpoint for a connection request from a remote peer
 *
 * @param    CompletionRoutine = Routine to be called when IRP is completed
 * @param    CompletionContext = Context for CompletionRoutine
 *
 * @return   Status of operation
 *           May return STATUS_PENDING
 */
{
    PDEVICE_OBJECT DeviceObject;

    DPRINT("Called\n");

    if (!AcceptConnectionObject) {
        DPRINT("Bad connection object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(AcceptConnectionObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_LISTEN,
                                            DeviceObject,
                                            AcceptConnectionObject,
                                            NULL,
                                            NULL);
    if (*Irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    TdiBuildAccept(*Irp,
                   DeviceObject,
                   AcceptConnectionObject,
                   CompletionRoutine,
                   CompletionContext,
                   RequestConnectionInfo,
                   ReturnConnectionInfo);

    TdiCall(*Irp, DeviceObject, NULL /* Don't wait for completion */, NULL);

    return STATUS_PENDING;
}


NTSTATUS TdiSetEventHandler(
    PFILE_OBJECT FileObject,
    LONG EventType,
    PVOID Handler,
    PVOID Context)
/*!
 * @brief Sets or resets an event handler
 *
 * @param    FileObject = Pointer to file object
 * @param    EventType  = Event code
 * @param    Handler    = Event handler to be called when the event occurs
 * @param    Context    = Context input to handler when the event occurs
 *
 * @return   Status of operation
 *
 * Specify NULL for Handler to stop calling event handler
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;

    DPRINT("Called\n");

    if (!FileObject) {
        DPRINT("Bad file object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_SET_EVENT_HANDLER,
                                           DeviceObject,
                                           FileObject,
                                           &Event,
                                           &Iosb);
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
/*!
 * @brief Queries a device for information
 *
 * @param    FileObject         = Pointer to file object
 * @param    IoControlCode      = I/O control code
 * @param    InputBuffer        = Pointer to buffer with input data
 * @param    InputBufferLength  = Length of InputBuffer
 * @param    OutputBuffer       = Address of buffer to place output data
 * @param    OutputBufferLength = Length of OutputBuffer
 *
 * @return   Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    KEVENT Event;
    PIRP Irp;

    if (!FileObject) {
        DPRINT("Bad file object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
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
/*!
 * @brief Query for information
 *
 * @param    FileObject   = Pointer to file object
 * @param    QueryType    = Query type
 * @param    MdlBuffer    = Pointer to MDL buffer specific for query type
 *
 * @return   Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;

    if (!FileObject) {
        DPRINT("Bad file object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_QUERY_INFORMATION,
                                           DeviceObject,
                                           ConnectionObject,
                                           &Event,
                                           &Iosb);
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
/*!
 * @brief Extended query for information
 *
 * @param    FileObject   = Pointer to file object
 * @param    Entity       = Entity
 * @param    Instance     = Instance
 * @param    Class        = Entity class
 * @param    Type         = Entity type
 * @param    Id           = Entity id
 * @param    OutputBuffer = Address of buffer to place data
 * @param    OutputLength = Address of buffer with length of OutputBuffer (updated)
 *
 * @return   Status of operation
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
/*!
 * @brief Queries for a local IP address
 *
 * @param    FileObject = Pointer to file object
 * @param    Address    = Address of buffer to place local address
 *
 * @return   Status of operation
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

    DPRINT("Called\n");

    BufferSize = sizeof(TDIEntityID) * 20;
    Entities = (TDIEntityID*)ExAllocatePoolWithTag(NonPagedPool,
                                                   BufferSize,
                                                   TAG_AFD_TRANSPORT_ADDRESS);
    if (!Entities) {
        DPRINT("Insufficient resources.\n");
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
        DPRINT("Unable to get list of supported entities (Status = 0x%X).\n", Status);
        ExFreePoolWithTag(Entities, TAG_AFD_TRANSPORT_ADDRESS);
        return Status;
    }

    /* Locate an IP entity */
    EntityCount = BufferSize / sizeof(TDIEntityID);

    DPRINT("EntityCount = %u\n", EntityCount);

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
                DPRINT("Unable to get entity of type IP (Status = 0x%X).\n", Status);
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
                DPRINT("Unable to get SNMP information or no IP addresses available (Status = 0x%X).\n", Status);
                break;
            }

            /* Query device for all IP addresses */

            if (SnmpInfo.ipsi_numaddr != 0) {
                BufferSize = SnmpInfo.ipsi_numaddr * sizeof(IPADDR_ENTRY);
                IpAddress = (PIPADDR_ENTRY)ExAllocatePoolWithTag(NonPagedPool,
                                                                 BufferSize,
                                                                 TAG_AFD_SNMP_ADDRESS_INFO);
                if (!IpAddress) {
                    DPRINT("Insufficient resources.\n");
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
                    DPRINT("Unable to get IP address (Status = 0x%X).\n", Status);
                    ExFreePoolWithTag(IpAddress, TAG_AFD_SNMP_ADDRESS_INFO);
                    break;
                }

                if (SnmpInfo.ipsi_numaddr != 1) {
                    /* Skip loopback address */
                    *Address = DN2H(IpAddress[1].Addr);
                } else {
                    /* Select the first address returned */
                    *Address = DN2H(IpAddress->Addr);
                }

                ExFreePoolWithTag(IpAddress, TAG_AFD_SNMP_ADDRESS_INFO);
            } else {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        }
    }

    ExFreePoolWithTag(Entities, TAG_AFD_TRANSPORT_ADDRESS);

    DPRINT("Leaving\n");

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

    if (!TransportObject) {
        DPRINT("Bad transport object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_SEND,
                                            DeviceObject,
                                            TransportObject,
                                            NULL,
                                            NULL);

    if (!*Irp) {
        DPRINT("Insufficient resources.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("Allocating irp for %p:%u\n", Buffer,BufferLength);

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */
    if (!Mdl) {
        DPRINT("Insufficient resources.\n");
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        MmProbeAndLockPages(Mdl, (*Irp)->RequestorMode, IoReadAccess);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        DPRINT("MmProbeAndLockPages() failed.\n");
        IoFreeMdl(Mdl);
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH2_END;

    DPRINT("AFD>>> Got an MDL: %p\n", Mdl);

    TdiBuildSend(*Irp,
                 DeviceObject,
                 TransportObject,
                 CompletionRoutine,
                 CompletionContext,
                 Mdl,
                 Flags,
                 BufferLength);

    /* Does not block...  The MDL is deleted in the receive completion
       routine. */
    return TdiCall(*Irp, DeviceObject, NULL, NULL);
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

    if (!TransportObject) {
        DPRINT("Bad transport object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_RECEIVE,
                                            DeviceObject,
                                            TransportObject,
                                            NULL,
                                            NULL);

    if (!*Irp) {
        DPRINT("Insufficient resources.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("Allocating irp for %p:%u\n", Buffer,BufferLength);

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */
    if (!Mdl) {
        DPRINT("Insufficient resources.\n");
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        DPRINT("probe and lock\n");
        MmProbeAndLockPages(Mdl, (*Irp)->RequestorMode, IoModifyAccess);
        DPRINT("probe and lock done\n");
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        DPRINT("MmProbeAndLockPages() failed.\n");
        IoFreeMdl(Mdl);
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH2_END;

    DPRINT("AFD>>> Got an MDL: %p\n", Mdl);

    TdiBuildReceive(*Irp,
                    DeviceObject,
                    TransportObject,
                    CompletionRoutine,
                    CompletionContext,
                    Mdl,
                    Flags,
                    BufferLength);


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
/*!
 * @brief Receives a datagram
 *
 * @param    TransportObject = Pointer to transport object
 * @param    From            = Receive filter (NULL if none)
 * @param    Address         = Address of buffer to place remote address
 * @param    Buffer          = Address of buffer to place received data
 * @param    BufferSize      = Address of buffer with length of Buffer (updated)
 *
 * @return   Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl;

    if (!TransportObject) {
        DPRINT("Bad tranport object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_RECEIVE_DATAGRAM,
                                            DeviceObject,
                                            TransportObject,
                                            NULL,
                                            NULL);

    if (!*Irp) {
        DPRINT("Insufficient resources.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("Allocating irp for %p:%u\n", Buffer,BufferLength);

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */
    if (!Mdl) {
        DPRINT("Insufficient resources.\n");
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        MmProbeAndLockPages(Mdl, (*Irp)->RequestorMode, IoModifyAccess);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        DPRINT("MmProbeAndLockPages() failed.\n");
        IoFreeMdl(Mdl);
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH2_END;

    DPRINT("AFD>>> Got an MDL: %p\n", Mdl);

    TdiBuildReceiveDatagram(*Irp,
                            DeviceObject,
                            TransportObject,
                            CompletionRoutine,
                            CompletionContext,
                            Mdl,
                            BufferLength,
                            Addr,
                            Addr,
                            Flags);

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
/*!
 * @brief Sends a datagram
 *
 * @param    TransportObject = Pointer to transport object
 * @param    From            = Send filter (NULL if none)
 * @param    Address         = Address of buffer to place remote address
 * @param    Buffer          = Address of buffer to place send data
 * @param    BufferSize      = Address of buffer with length of Buffer (updated)
 *
 * @return   Status of operation
 */
{
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl;

    if (!TransportObject) {
        DPRINT("Bad transport object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DPRINT("Called(TransportObject %p)\n", TransportObject);

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (BufferLength == 0)
    {
        DPRINT("Succeeding send with length 0.\n");
        return STATUS_SUCCESS;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_SEND_DATAGRAM,
                                            DeviceObject,
                                            TransportObject,
                                            NULL,
                                            NULL);

    if (!*Irp) {
        DPRINT("Insufficient resources.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("Allocating irp for %p:%u\n", Buffer,BufferLength);

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */

    if (!Mdl) {
        DPRINT("Insufficient resources.\n");
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        MmProbeAndLockPages(Mdl, (*Irp)->RequestorMode, IoReadAccess);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        DPRINT("MmProbeAndLockPages() failed.\n");
        IoFreeMdl(Mdl);
        IoCompleteRequest(*Irp, IO_NO_INCREMENT);
        *Irp = NULL;
        _SEH2_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH2_END;

    DPRINT("AFD>>> Got an MDL: %p\n", Mdl);

    TdiBuildSendDatagram(*Irp,
                         DeviceObject,
                         TransportObject,
                         CompletionRoutine,
                         CompletionContext,
                         Mdl,
                         BufferLength,
                         Addr);

    /* Does not block...  The MDL is deleted in the send completion
       routine. */
    return TdiCall(*Irp, DeviceObject, NULL, NULL);
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
        DPRINT("Bad transport object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    DPRINT("Called(TransportObject %p)\n", TransportObject);

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        DPRINT("Bad device object.\n");
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp(TDI_DISCONNECT,
                                            DeviceObject,
                                            TransportObject,
                                            NULL,
                                            NULL);

    if (!*Irp) {
        DPRINT("Insufficient resources.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildDisconnect(*Irp,
                       DeviceObject,
                       TransportObject,
                       CompletionRoutine,
                       CompletionContext,
                       Time,
                       Flags,
                       RequestConnectionInfo,
                       ReturnConnectionInfo);

    TdiCall(*Irp, DeviceObject, NULL, NULL);

    return STATUS_PENDING;
}

/* EOF */
