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

inline DWORD TdiAddressSizeFromName(
    LPSOCKADDR Name)
/*
 * FUNCTION: Returns the size of a TDI style address equivalent to a
 *           WinSock style name
 * ARGUMENTS:
 *     Name = WinSock style name
 * RETURNS:
 *     Size of TDI style address, 0 if Name is not valid
 */
{
    switch (Name->sa_family) {
    case AF_INET:
        return sizeof(TA_ADDRESS_IP);
    /* FIXME: More to come */
    }
    AFD_DbgPrint(MIN_TRACE, ("Unknown address family (%d).\n", Name->sa_family));
    return 0;
}


VOID TdiBuildAddressIPv4(
    PTA_ADDRESS_IP Address,
    LPSOCKADDR Name)
/*
 * FUNCTION: Builds an IPv4 TDI style address
 * ARGUMENTS:
 *     Address = Address of buffer to place TDI style IPv4 address
 *     Name    = Pointer to WinSock style IPv4 name
 */
{
    Address->TAAddressCount                 = 1;
    Address->Address[0].AddressLength       = TDI_ADDRESS_LENGTH_IP;
    Address->Address[0].AddressType         = TDI_ADDRESS_TYPE_IP;
    Address->Address[0].Address[0].sin_port = ((LPSOCKADDR_IN)Name)->sin_port;
    Address->Address[0].Address[0].in_addr  = ((LPSOCKADDR_IN)Name)->sin_addr.S_un.S_addr;
}


VOID TdiBuildAddress(
    PTA_ADDRESS Address,
    LPSOCKADDR Name)
/*
 * FUNCTION: Builds a TDI style address
 * ARGUMENTS:
 *     Address = Address of buffer to place TDI style address
 *     Name    = Pointer to WinSock style name
 */
{
    switch (Name->sa_family) {
    case AF_INET:
        TdiBuildAddressIPv4((PTA_ADDRESS_IP)Address, Name);
        break;
    /* FIXME: More to come */
    default:
        AFD_DbgPrint(MIN_TRACE, ("Unknown address family (%d).\n", Name->sa_family));
    }
}


VOID TdiBuildName(
    LPSOCKADDR Name,
    PTA_ADDRESS Address)
/*
 * FUNCTION: Builds a WinSock style address
 * ARGUMENTS:
 *     Name    = Address of buffer to place WinSock style name
 *     Address = Pointer to TDI style address
 */
{
    switch (Address->AddressType) {
    case TDI_ADDRESS_TYPE_IP:
        Name->sa_family = AF_INET;
        ((LPSOCKADDR_IN)Name)->sin_port = 
            ((PTDI_ADDRESS_IP)&Address->Address[0])->sin_port;
        ((LPSOCKADDR_IN)Name)->sin_addr.S_un.S_addr = 
            ((PTDI_ADDRESS_IP)&Address->Address[0])->in_addr;
        return;
    /* FIXME: More to come */
    }
    AFD_DbgPrint(MIN_TRACE, ("Unknown TDI address type (%d).\n", Address->AddressType));
}


NTSTATUS TdiCall(
    PIRP Irp,
    PDEVICE_OBJECT DeviceObject,
    PIO_STATUS_BLOCK IoStatusBlock,
    BOOLEAN CanCancel,
    PKEVENT StopEvent)
/*
 * FUNCTION: Calls a transport driver device
 * ARGUMENTS:
 *     Irp           = Pointer to I/O Request Packet
 *     DeviceObject  = Pointer to device object to call
 *     IoStatusBlock = Address of buffer with I/O status block
 *     CanCancel     = TRUE if the IRP can be cancelled, FALSE if not
 *     StopEvent     = If CanCancel is TRUE, a pointer to an event handle
 *                     that, when signalled will cause the request to abort
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     All requests are completed synchronously. A request can be cancelled
 */
{
    KEVENT Event;
    PKEVENT Events[2];
    NTSTATUS Status;
    Events[0] = StopEvent;
    Events[1] = &Event; 

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp->UserEvent = &Event;
    Irp->UserIosb  = IoStatusBlock;
    Status         = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING) {
        if (CanCancel) {
            Status = KeWaitForMultipleObjects(2,
                                              (PVOID)&Events,
                                              WaitAny,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL,
                                              NULL);

            if (KeReadStateEvent(StopEvent) != 0) {
                if (IoCancelIrp(Irp)) {
                    AFD_DbgPrint(MAX_TRACE, ("Cancelled IRP.\n"));
                } else {
                    AFD_DbgPrint(MIN_TRACE, ("Could not cancel IRP.\n"));
                }
                return STATUS_CANCELLED;
            }
        } else
            Status = KeWaitForSingleObject(&Event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);
    }

    AFD_DbgPrint(MIN_TRACE, ("Status (0x%X)  Iosb.Status (0x%X).\n", Status, IoStatusBlock->Status));

    return IoStatusBlock->Status;
}


NTSTATUS TdiOpenDevice(
    PUNICODE_STRING DeviceName,
    ULONG EaLength,
    PFILE_FULL_EA_INFORMATION EaInfo,
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

    InitializeObjectAttributes(&Attr,                   /* Attribute buffer */
                               DeviceName,              /* Device name */
                               OBJ_CASE_INSENSITIVE,    /* Attributes */
                               NULL,                    /* Root directory */
                               NULL);                   /* Security descriptor */

    Status = ZwCreateFile(Handle,                               /* Return file handle */
                          GENERIC_READ | GENERIC_WRITE,         /* Desired access */
                          &Attr,                                /* Object attributes */
                          &Iosb,                                /* IO status */
                          0,                                    /* Initial allocation size */
                          FILE_ATTRIBUTE_NORMAL,                /* File attributes */
                          FILE_SHARE_READ | FILE_SHARE_WRITE,   /* Share access */
                          FILE_OPEN_IF,                         /* Create disposition */
                          0,                                    /* Create options */
                          EaInfo,                               /* EA buffer */
                          EaLength);                            /* EA length */
    if (NT_SUCCESS(Status)) {
        Status  = ObReferenceObjectByHandle(*Handle,                        /* Handle to open file */
                                            GENERIC_READ | GENERIC_WRITE,   /* Access mode */
                                            NULL,                           /* Object type */
                                            KernelMode,                     /* Access mode */
                                            (PVOID*)Object,                 /* Pointer to object */
                                            NULL);                          /* Handle information */
        if (!NT_SUCCESS(Status)) {
            AFD_DbgPrint(MIN_TRACE, ("ObReferenceObjectByHandle() failed with status (0x%X).\n", Status));
            ZwClose(*Handle);
        }
    } else {
        AFD_DbgPrint(MIN_TRACE, ("ZwCreateFile() failed with status (0x%X)\n", Status));
    }

    return Status;
}


NTSTATUS TdiCloseDevice(
    HANDLE Handle,
    PFILE_OBJECT FileObject)
{
    if (FileObject)
        ObDereferenceObject(FileObject);

    if (Handle)
        ZwClose(Handle);

    return STATUS_SUCCESS;
}


NTSTATUS TdiOpenAddressFileIPv4(
    PUNICODE_STRING DeviceName,
    LPSOCKADDR Name,
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
    PTA_ADDRESS_IP Address;
    NTSTATUS Status;
    ULONG EaLength;

    EaLength = sizeof(FILE_FULL_EA_INFORMATION) +
               TDI_TRANSPORT_ADDRESS_LENGTH +
               sizeof(TA_ADDRESS_IP);
    EaInfo = (PFILE_FULL_EA_INFORMATION)ExAllocatePool(NonPagedPool, EaLength);
    if (!EaInfo) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(EaInfo, EaLength);
    EaInfo->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    RtlCopyMemory(EaInfo->EaName,
                  TdiTransportAddress,
                  TDI_TRANSPORT_ADDRESS_LENGTH);
    EaInfo->EaValueLength = sizeof(TA_ADDRESS_IP);
    Address = (PTA_ADDRESS_IP)(EaInfo->EaName + TDI_TRANSPORT_ADDRESS_LENGTH);
    TdiBuildAddressIPv4(Address, Name);

    Status = TdiOpenDevice(DeviceName,
                           EaLength,
                           EaInfo,
                           AddressHandle,
                           AddressObject);
    ExFreePool(EaInfo);
    return Status;
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
    NTSTATUS Status;
    PIRP Irp;

    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_SET_EVENT_HANDLER,   /* Sub function */
                                           DeviceObject,            /* Device object */
                                           FileObject,              /* File object */
                                           NULL,                    /* Event */
                                           NULL);                   /* Status */
    if (!Irp) {
        AFD_DbgPrint(MIN_TRACE, ("TdiBuildInternalDeviceControlIrp() failed.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildSetEventHandler(Irp,
                            DeviceObject,
                            FileObject,
                            NULL,
                            NULL,
                            EventType,
                            Handler,
                            Context);

    Status = TdiCall(Irp, DeviceObject, &Iosb, FALSE, NULL);

    return Status;
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
    PIRP Irp;

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE,
                                        NULL,
                                        NULL);
    if (!Irp) {
        AFD_DbgPrint(MIN_TRACE, ("IoBuildDeviceIoControlRequest() failed.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = TdiCall(Irp, DeviceObject, &Iosb, FALSE, NULL);
    if (Return)
        *Return = Iosb.Information;

    return Status;
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
    IPSNMP_INFO SnmpInfo;
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

    AFD_DbgPrint(MAX_TRACE, ("EntityCount = %d\n", EntityCount));

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
            if (!NT_SUCCESS(Status) || (SnmpInfo.NumAddr == 0)) {
                AFD_DbgPrint(MIN_TRACE, ("Unable to get SNMP information or no IP addresses available (Status = 0x%X).\n", Status));
                break;
            }

            /* Query device for all IP addresses */

            if (SnmpInfo.NumAddr != 0) {
                BufferSize = SnmpInfo.NumAddr * sizeof(IPADDR_ENTRY);
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

                if (SnmpInfo.NumAddr != 1) {
                    /* Skip loopback address */
                    *Address = DN2H(((PIPADDR_ENTRY)((ULONG)IpAddress + sizeof(IPADDR_ENTRY)))->Addr);
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
    PFILE_OBJECT TransportObject,
    PFILE_REQUEST_SENDTO Request)
/*
 * FUNCTION: Sends a block of data
 * ARGUMENTS:
 *     TransportObject = Pointer to transport object
 *     Request         = Pointer to request
 * RETURNS:
 *     Status of operation
 */
{
    PTDI_CONNECTION_INFORMATION ConnectInfo;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    DWORD TdiAddressSize;
    ULONG BufferSize;
    NTSTATUS Status;
    PIRP Irp;
    PMDL Mdl;

    /* FIXME: Connectionless only */

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    TdiAddressSize = TdiAddressSizeFromName(&Request->To);

    ConnectInfo  = (PTDI_CONNECTION_INFORMATION)
        ExAllocatePool(NonPagedPool,
        sizeof(TDI_CONNECTION_INFORMATION) +
        TdiAddressSize);

    if (!ConnectInfo) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(ConnectInfo,
        sizeof(TDI_CONNECTION_INFORMATION) +
        TdiAddressSize);

    ConnectInfo->RemoteAddressLength = TdiAddressSize;
    ConnectInfo->RemoteAddress       = (PVOID)
        (ConnectInfo + sizeof(TDI_CONNECTION_INFORMATION));

    TdiBuildAddress(ConnectInfo->RemoteAddress, &Request->To);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_SEND_DATAGRAM,   /* Sub function */
                                           DeviceObject,        /* Device object */
                                           TransportObject,     /* File object */
                                           NULL,                /* Event */
                                           NULL);               /* Return buffer */
    if (!Irp) {
        AFD_DbgPrint(MIN_TRACE, ("TdiBuildInternalDeviceControlIrp() failed.\n"));
        ExFreePool(ConnectInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* FIXME: There may be more than one buffer */
    BufferSize = Request->Buffers->len;
    Mdl = IoAllocateMdl(
        Request->Buffers->buf,  /* Virtual address of buffer */
        Request->Buffers->len,  /* Length of buffer */
        FALSE,                  /* Not secondary */
        FALSE,                  /* Don't charge quota */
        NULL);                  /* Don't use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("IoAllocateMdl() failed.\n"));
        IoFreeIrp(Irp);
        ExFreePool(ConnectInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#ifdef _MSC_VER
    try {
#endif
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
#ifdef _MSC_VER
    } except(EXCEPTION_EXECUTE_HANDLER) {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
        IoFreeMdl(Mdl);
        IoFreeIrp(Irp);
        ExFreePool(ConnectInfo);
        return STATUS_UNSUCCESSFUL;
    }
#endif

    TdiBuildSendDatagram(Irp,               /* I/O Request Packet */
                         DeviceObject,      /* Device object */
                         TransportObject,   /* File object */
                         NULL,              /* Completion routine */
                         NULL,              /* Completion context */
                         Mdl,               /* Descriptor for data buffer */
                         BufferSize,        /* Size of data to send */
                         ConnectInfo);      /* Connection information */

    Status = TdiCall(Irp, DeviceObject, &Iosb, FALSE, NULL);

    ExFreePool(ConnectInfo);

    return Status;
}


NTSTATUS TdiSendDatagram(
    PFILE_OBJECT TransportObject,
    LPSOCKADDR Address,
    PVOID Buffer,
    ULONG BufferSize)
/*
 * FUNCTION: Sends a datagram
 * ARGUMENTS:
 *     TransportObject = Pointer to transport object
 *     Address         = Remote address
 *     Buffer          = Pointer to buffer with data to send
 *     BufferSize      = Length of Buffer
 * RETURNS:
 *     Status of operation
 */
{
    PTDI_CONNECTION_INFORMATION ConnectInfo;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    DWORD TdiAddressSize;
    NTSTATUS Status;
    PIRP Irp;
    PMDL Mdl;

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    TdiAddressSize = TdiAddressSizeFromName(Address);

    ConnectInfo  = (PTDI_CONNECTION_INFORMATION)
        ExAllocatePool(NonPagedPool,
        sizeof(TDI_CONNECTION_INFORMATION) +
        TdiAddressSize);

    if (!ConnectInfo) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(ConnectInfo,
        sizeof(TDI_CONNECTION_INFORMATION) +
        TdiAddressSize);

    ConnectInfo->RemoteAddressLength = TdiAddressSize;
    ConnectInfo->RemoteAddress       = (PVOID)
        (ConnectInfo + sizeof(TDI_CONNECTION_INFORMATION));

    TdiBuildAddress(ConnectInfo->RemoteAddress, Address);

    Irp = TdiBuildInternalDeviceControlIrp(TDI_SEND_DATAGRAM,   /* Sub function */
                                           DeviceObject,        /* Device object */
                                           TransportObject,     /* File object */
                                           NULL,                /* Event */
                                           NULL);               /* Return buffer */
    if (!Irp) {
        AFD_DbgPrint(MIN_TRACE, ("TdiBuildInternalDeviceControlIrp() failed.\n"));
        ExFreePool(ConnectInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Mdl = IoAllocateMdl(Buffer,     /* Virtual address of buffer */
                        BufferSize, /* Length of buffer */
                        FALSE,      /* Not secondary */
                        FALSE,      /* Don't charge quota */
                        NULL);      /* Don't use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("IoAllocateMdl() failed.\n"));
        IoFreeIrp(Irp);
        ExFreePool(ConnectInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#ifdef _MSC_VER
    try {
#endif
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
#ifdef _MSC_VER
    } except(EXCEPTION_EXECUTE_HANDLER) {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
        IoFreeMdl(Mdl);
        IoFreeIrp(Irp);
        ExFreePool(ConnectInfo);
        return STATUS_UNSUCCESSFUL;
    }
#endif

    TdiBuildSendDatagram(Irp,               /* I/O Request Packet */
                         DeviceObject,      /* Device object */
                         TransportObject,   /* File object */
                         NULL,              /* Completion routine */
                         NULL,              /* Completion context */
                         Mdl,               /* Descriptor for data buffer */
                         BufferSize,        /* Size of data to send */
                         ConnectInfo);      /* Connection information */

    Status = TdiCall(Irp, DeviceObject, &Iosb, FALSE, NULL);

    ExFreePool(ConnectInfo);

    return Status;
}


NTSTATUS TdiReceiveDatagram(
    PFILE_OBJECT TransportObject,
    LPSOCKADDR From,
    LPSOCKADDR Address,
    PUCHAR Buffer,
    PULONG BufferSize)
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
    PTDI_CONNECTION_INFORMATION ReceiveInfo;
    PTDI_CONNECTION_INFORMATION ReturnInfo;
    PTA_ADDRESS_IP ReturnAddress;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    DWORD TdiAddressSize;
    NTSTATUS Status;
    PIRP Irp;
    PMDL Mdl;

    if (From != NULL) {
        /* FIXME: Check that the socket type match the socket */
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME: Get from socket information */
    TdiAddressSize = sizeof(TA_ADDRESS_IP);

    ReceiveInfo = (PTDI_CONNECTION_INFORMATION)
        ExAllocatePool(NonPagedPool,
                       sizeof(TDI_CONNECTION_INFORMATION) +
                       sizeof(TDI_CONNECTION_INFORMATION) +
                       2 * TdiAddressSize);
    if (!ReceiveInfo) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(ReceiveInfo,
                  sizeof(TDI_CONNECTION_INFORMATION) +
                  sizeof(TDI_CONNECTION_INFORMATION) +
                  2 * TdiAddressSize);

    if (From != NULL) {
        ReceiveInfo->RemoteAddressLength = TdiAddressSize;
        ReceiveInfo->RemoteAddress       = (PVOID)
            (ReceiveInfo + sizeof(TDI_CONNECTION_INFORMATION));
        /* Filter datagrams */
        TdiBuildAddress(ReceiveInfo->RemoteAddress, From);
    } else {
        /* Receive from any address */
        ReceiveInfo->RemoteAddressLength = 0;
        ReceiveInfo->RemoteAddress       = NULL;
    }

    ReturnInfo = (PTDI_CONNECTION_INFORMATION)
        (ReceiveInfo + sizeof(TDI_CONNECTION_INFORMATION) + TdiAddressSize);
    ReturnInfo->RemoteAddressLength = TdiAddressSize;
    ReturnInfo->RemoteAddress       = (PVOID)
        (ReturnInfo + sizeof(TDI_CONNECTION_INFORMATION));

    Irp = TdiBuildInternalDeviceControlIrp(TDI_RECEIVE_DATAGRAM,    /* Sub function */
                                           DeviceObject,            /* Device object */
                                           TransportObject,         /* File object */
                                           NULL,                    /* Event */
                                           NULL);                   /* Return buffer */
    if (!Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        ExFreePool(ReceiveInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        *BufferSize,    /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        NULL);          /* Don't use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoFreeIrp(Irp);
        ExFreePool(ReceiveInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#ifdef _MSC_VER
    try {
#endif
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
#ifdef _MSC_VER
    } except (EXCEPTION_EXECUTE_HANDLER) {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
        IoFreeMdl(Mdl);
        IoFreeIrp(Irp);
        ExFreePool(ReceiveInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#endif

    TdiBuildReceiveDatagram(Irp,                    /* I/O Request Packet */
                            DeviceObject,           /* Device object */
                            TransportObject,        /* File object */
                            NULL,                   /* Completion routine */
                            NULL,                   /* Completion context */
                            Mdl,                    /* Data buffer */
                            *BufferSize,            /* Size of data buffer */
                            ReceiveInfo,            /* Connection information */
                            ReturnInfo,             /* Connection information */
                            TDI_RECEIVE_NORMAL);    /* Flags */
    Status = TdiCall(Irp, DeviceObject, &Iosb, TRUE, NULL);
    if (NT_SUCCESS(Status)) {
        *BufferSize = Iosb.Information;
        TdiBuildName(Address, ReturnInfo->RemoteAddress);
    }

    IoFreeMdl(Mdl);
    ExFreePool(ReceiveInfo);

    return Status;
}

/* EOF */
