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
#include <pseh/pseh.h>
#include "debug.h"
#include "tdiconn.h"
#include "tdi_proto.h"

#ifdef DBG
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

    AFD_DbgPrint(MIN_TRACE, ("Displaying buffer at (0x%X)  Size (%d).\n", Buffer, Size));

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

    AFD_DbgPrint(MID_TRACE, ("Irp->UserEvent = %x\n", Irp->UserEvent));

    Status = IoCallDriver(DeviceObject, Irp);
    AFD_DbgPrint(MID_TRACE, ("IoCallDriver: %08x\n", Status));

    if ((Status == STATUS_PENDING) && (Event != NULL)) {
        AFD_DbgPrint(MAX_TRACE, ("Waiting on transport.\n"));
        KeWaitForSingleObject(
          Event,
          Executive,
          UserMode,
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

    AFD_DbgPrint(MAX_TRACE, ("Called. DeviceName (%wZ)\n", DeviceName));

    InitializeObjectAttributes(&Attr,                   /* Attribute buffer */
                               DeviceName,              /* Device name */
                               OBJ_CASE_INSENSITIVE,    /* Attributes */
                               NULL,                    /* Root directory */
                               NULL);                   /* Security descriptor */

    Status = ZwCreateFile(Handle,                               /* Return file handle */
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,         /* Desired access */
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
        Status = ObReferenceObjectByHandle(*Handle,                       /* Handle to open file */
                                           GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,  /* Access mode */
                                           NULL,                          /* Object type */
                                           KernelMode,                    /* Access mode */
                                           (PVOID*)Object,                /* Pointer to object */
                                           NULL);                         /* Handle information */
        if (!NT_SUCCESS(Status)) {
          AFD_DbgPrint(MIN_TRACE, ("ObReferenceObjectByHandle() failed with status (0x%X).\n", Status));
          ZwClose(*Handle);
          *Handle = NULL;
        } else {
          AFD_DbgPrint(MAX_TRACE, ("Got handle (0x%X)  Object (0x%X)\n",
            *Handle, *Object));
        }
    } else {
        AFD_DbgPrint(MIN_TRACE, ("ZwCreateFile() failed with status (0x%X)\n", Status));
        *Handle = NULL;
    }

    return Status;
}


NTSTATUS TdiCloseDevice(
    HANDLE Handle,
    PFILE_OBJECT FileObject)
{
    AFD_DbgPrint(MAX_TRACE, ("Called. Handle (0x%X)  FileObject (0x%X)\n",
      Handle, FileObject));

    if (Handle)
        ZwClose(Handle);

    if (FileObject)
        ObDereferenceObject(FileObject);

    return STATUS_SUCCESS;
}


NTSTATUS TdiOpenAddressFile(
    PUNICODE_STRING DeviceName,
    PTRANSPORT_ADDRESS Name,
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

  AFD_DbgPrint(MAX_TRACE, ("Called. DeviceName (%wZ)  Name (0x%X)\n",
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
                         AddressHandle,
                         AddressObject);
  ExFreePool(EaInfo);
  return Status;
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
                         ConnectionHandle,
                         ConnectionObject);
  ExFreePool(EaInfo);
  return Status;
}


NTSTATUS TdiConnect(
    PIRP *Irp,
    PFILE_OBJECT ConnectionObject,
    PTDI_CONNECTION_INFORMATION RemoteAddress,
    PIO_STATUS_BLOCK Iosb,
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
  NTSTATUS Status;

  AFD_DbgPrint(MAX_TRACE, ("Called\n"));

  if (!ConnectionObject) {
	AFD_DbgPrint(MIN_TRACE, ("Bad connection object.\n"));
	*Irp = NULL;
	return STATUS_INVALID_PARAMETER;
  }

  DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
  if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        *Irp = NULL;
        return STATUS_INVALID_PARAMETER;
  }

  *Irp = TdiBuildInternalDeviceControlIrp(TDI_CONNECT,             /* Sub function */
					  DeviceObject,            /* Device object */
					  ConnectionObject,        /* File object */
					  NULL,                    /* Event */
					  Iosb);                   /* Status */
  if (!*Irp) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  TdiBuildConnect(*Irp,                   /* IRP */
                  DeviceObject,           /* Device object */
                  ConnectionObject,       /* File object */
                  CompletionRoutine,      /* Completion routine */
                  CompletionContext,      /* Completion routine context */
                  NULL,                   /* Time */
                  RemoteAddress,          /* Request connection information */
                  RemoteAddress);         /* Return connection information */

  Status = TdiCall(*Irp, DeviceObject, NULL, Iosb);

  return Status;
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
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  AFD_DbgPrint(MAX_TRACE, ("Called. AddressHandle (0x%X)  ConnectionObject (0x%X)\n",
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

  Status = TdiCall(Irp, DeviceObject, &Event, &Iosb);

  return Status;
}


NTSTATUS TdiListen
( PIRP *Irp,
  PFILE_OBJECT ConnectionObject,
  PTDI_CONNECTION_INFORMATION *RequestConnectionInfo,
  PTDI_CONNECTION_INFORMATION *ReturnConnectionInfo,
  PIO_STATUS_BLOCK Iosb,
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
  NTSTATUS Status;

  AFD_DbgPrint(MAX_TRACE, ("Called\n"));

  if (!ConnectionObject) {
	AFD_DbgPrint(MIN_TRACE, ("Bad connection object.\n"));
	*Irp = NULL;
	return STATUS_INVALID_PARAMETER;
  }

  DeviceObject = IoGetRelatedDeviceObject(ConnectionObject);
  if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        *Irp = NULL;
        return STATUS_INVALID_PARAMETER;
  }

  Status = TdiBuildNullConnectionInfo(RequestConnectionInfo,
				      TDI_ADDRESS_TYPE_IP);
  if (!NT_SUCCESS(Status))
    return Status;

  *Irp = TdiBuildInternalDeviceControlIrp(TDI_LISTEN,              /* Sub function */
					  DeviceObject,            /* Device object */
					  ConnectionObject,        /* File object */
					  NULL,                    /* Event */
					  Iosb);                   /* Status */
  if (*Irp == NULL)
    {
	ExFreePool(*RequestConnectionInfo);
	return STATUS_INSUFFICIENT_RESOURCES;
    }

  TdiBuildListen(*Irp,                   /* IRP */
                 DeviceObject,           /* Device object */
                 ConnectionObject,       /* File object */
                 CompletionRoutine,      /* Completion routine */
                 CompletionContext,      /* Completion routine context */
                 0,                      /* Flags */
                 *RequestConnectionInfo, /* Request connection information */
		 *ReturnConnectionInfo);  /* Return connection information */

  Status = TdiCall(*Irp, DeviceObject, NULL /* Don't wait for completion */, Iosb);

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

  Status = TdiCall(Irp, DeviceObject, &Event, &Iosb);

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

    Irp = TdiBuildInternalDeviceControlIrp(TDI_QUERY_INFORMATION,       /* Sub function */
                                           DeviceObject,                /* Device object */
                                           ConnectionObject,            /* File object */
                                           &Event,                      /* Event */
                                           &Iosb);                      /* Status */
    if (!Irp) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildQueryInformation(
      Irp,
      DeviceObject,
      FileObject,
      NULL,
      NULL,
      QueryType,
      MdlBuffer);

    Status = TdiCall(Irp, DeviceObject, &Event, &Iosb);

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
		    PIPADDR_ENTRY IpAddressEntry = (PIPADDR_ENTRY)
			((PCHAR)IpAddress) + sizeof(IPADDR_ENTRY);
                    *Address = DN2H(IpAddressEntry->Addr);
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

NTSTATUS TdiSend
( PIRP *Irp,
  PFILE_OBJECT TransportObject,
  USHORT Flags,
  PCHAR Buffer,
  UINT BufferLength,
  PIO_STATUS_BLOCK Iosb,
  PIO_COMPLETION_ROUTINE CompletionRoutine,
  PVOID CompletionContext )
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PMDL Mdl;

    if (!TransportObject) {
	AFD_DbgPrint(MIN_TRACE, ("Bad transport object.\n"));
	*Irp = NULL;
	return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        *Irp = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp
	( TDI_SEND,                /* Sub function */
	  DeviceObject,            /* Device object */
	  TransportObject,         /* File object */
	  NULL,                    /* Event */
	  Iosb );                  /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE, ("Allocating irp for %x:%d\n", Buffer,BufferLength));

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        *Irp);          /* use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoFreeIrp(*Irp);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH_TRY {
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
    } _SEH_HANDLE {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	IoFreeMdl(Mdl);
        IoFreeIrp(*Irp);
        *Irp = NULL;
        _SEH_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH_END;

    AFD_DbgPrint(MID_TRACE,("AFD>>> Got an MDL: %x\n", Mdl));

    TdiBuildSend(*Irp,                   /* I/O Request Packet */
		 DeviceObject,           /* Device object */
		 TransportObject,        /* File object */
		 CompletionRoutine,      /* Completion routine */
		 CompletionContext,      /* Completion context */
		 Mdl,                    /* Data buffer */
		 Flags,                  /* Flags */
		 BufferLength);          /* Length of data */

    Status = TdiCall(*Irp, DeviceObject, NULL, Iosb);
    /* Does not block...  The MDL is deleted in the receive completion
       routine. */

    return Status;
}

NTSTATUS TdiReceive(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PIO_STATUS_BLOCK Iosb,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject;
    PMDL Mdl;

    if (!TransportObject) {
	AFD_DbgPrint(MIN_TRACE, ("Bad transport object.\n"));
	*Irp = NULL;
	return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        *Irp = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp
	( TDI_RECEIVE,             /* Sub function */
	  DeviceObject,            /* Device object */
	  TransportObject,         /* File object */
	  NULL,                    /* Event */
	  Iosb );                  /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE, ("Allocating irp for %x:%d\n", Buffer,BufferLength));

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        *Irp);          /* Don't use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoFreeIrp(*Irp);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH_TRY {
        AFD_DbgPrint(MIN_TRACE, ("probe and lock\n"));
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
        AFD_DbgPrint(MIN_TRACE, ("probe and lock done\n"));
    } _SEH_HANDLE {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	IoFreeMdl(Mdl);
        IoFreeIrp(*Irp);
	*Irp = NULL;
	_SEH_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH_END;

    AFD_DbgPrint(MID_TRACE,("AFD>>> Got an MDL: %x\n", Mdl));

    TdiBuildReceive(*Irp,                   /* I/O Request Packet */
		    DeviceObject,           /* Device object */
		    TransportObject,        /* File object */
		    CompletionRoutine,      /* Completion routine */
		    CompletionContext,      /* Completion context */
		    Mdl,                    /* Data buffer */
		    Flags,                  /* Flags */
		    BufferLength);          /* Length of data */


    Status = TdiCall(*Irp, DeviceObject, NULL, Iosb);
    /* Does not block...  The MDL is deleted in the receive completion
       routine. */

    AFD_DbgPrint(MID_TRACE,("Status %x Information %d\n",
			    Status, Iosb->Information));

    return Status;
}


NTSTATUS TdiReceiveDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    USHORT Flags,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION Addr,
    PIO_STATUS_BLOCK Iosb,
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
    NTSTATUS Status;
    PMDL Mdl;

    if (!TransportObject) {
	AFD_DbgPrint(MIN_TRACE, ("Bad tranport object.\n"));
	*Irp = NULL;
	return STATUS_INVALID_PARAMETER;
    }

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        *Irp = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp
	( TDI_RECEIVE_DATAGRAM,    /* Sub function */
	  DeviceObject,            /* Device object */
	  TransportObject,         /* File object */
	  NULL,                    /* Event */
	  Iosb );                  /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE, ("Allocating irp for %x:%d\n", Buffer,BufferLength));

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        *Irp);          /* Don't use IRP */
    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoFreeIrp(*Irp);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH_TRY {
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
    } _SEH_HANDLE {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	IoFreeMdl(Mdl);
        IoFreeIrp(*Irp);
        *Irp = NULL;
        _SEH_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH_END;

    AFD_DbgPrint(MID_TRACE,("AFD>>> Got an MDL: %x\n", Mdl));

    TdiBuildReceiveDatagram
	(*Irp,                   /* I/O Request Packet */
	 DeviceObject,           /* Device object */
	 TransportObject,        /* File object */
	 CompletionRoutine,      /* Completion routine */
	 CompletionContext,      /* Completion context */
	 Mdl,                    /* Data buffer */
	 BufferLength,
	 Addr,
	 Addr,
	 Flags);                 /* Length of data */

    Status = TdiCall(*Irp, DeviceObject, NULL, Iosb);
    /* Does not block...  The MDL is deleted in the receive completion
       routine. */

    return Status;
}


NTSTATUS TdiSendDatagram(
    PIRP *Irp,
    PFILE_OBJECT TransportObject,
    PCHAR Buffer,
    UINT BufferLength,
    PTDI_CONNECTION_INFORMATION Addr,
    PIO_STATUS_BLOCK Iosb,
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
    NTSTATUS Status;
    PMDL Mdl;

    if (!TransportObject) {
	AFD_DbgPrint(MIN_TRACE, ("Bad transport object.\n"));
	*Irp = NULL;
	return STATUS_INVALID_PARAMETER;
    }

    AFD_DbgPrint(MID_TRACE,("Called(TransportObject %x)\n", TransportObject));

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        *Irp = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    *Irp = TdiBuildInternalDeviceControlIrp
	( TDI_SEND_DATAGRAM,       /* Sub function */
	  DeviceObject,            /* Device object */
	  TransportObject,         /* File object */
	  NULL,                    /* Event */
	  Iosb );                  /* Status */

    if (!*Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AFD_DbgPrint(MID_TRACE, ("Allocating irp for %x:%d\n", Buffer,BufferLength));

    Mdl = IoAllocateMdl(Buffer,         /* Virtual address */
                        BufferLength,   /* Length of buffer */
                        FALSE,          /* Not secondary */
                        FALSE,          /* Don't charge quota */
                        *Irp);          /* Don't use IRP */

    if (!Mdl) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        IoFreeIrp(*Irp);
        *Irp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH_TRY {
        MmProbeAndLockPages(Mdl, KernelMode, IoModifyAccess);
    } _SEH_HANDLE {
        AFD_DbgPrint(MIN_TRACE, ("MmProbeAndLockPages() failed.\n"));
	IoFreeMdl(Mdl);
        IoFreeIrp(*Irp);
        *Irp = NULL;
        _SEH_YIELD(return STATUS_INSUFFICIENT_RESOURCES);
    } _SEH_END;

    AFD_DbgPrint(MID_TRACE,("AFD>>> Got an MDL: %x\n", Mdl));

    TdiBuildSendDatagram
	(*Irp,                   /* I/O Request Packet */
	 DeviceObject,           /* Device object */
	 TransportObject,        /* File object */
	 CompletionRoutine,      /* Completion routine */
	 CompletionContext,      /* Completion context */
	 Mdl,                    /* Data buffer */
	 BufferLength,           /* Bytes to send */
	 Addr);                  /* Address */

    Status = TdiCall(*Irp, DeviceObject, NULL, Iosb);
    /* Does not block...  The MDL is deleted in the send completion
       routine. */

    return Status;
}

NTSTATUS TdiDisconnect(
    PFILE_OBJECT TransportObject,
    PLARGE_INTEGER Time,
    USHORT Flags,
    PIO_STATUS_BLOCK Iosb,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext,
    PTDI_CONNECTION_INFORMATION RequestConnectionInfo,
    PTDI_CONNECTION_INFORMATION ReturnConnectionInfo) {
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    KEVENT Event;
    PIRP Irp;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    if (!TransportObject) {
	AFD_DbgPrint(MIN_TRACE, ("Bad transport object.\n"));
	return STATUS_INVALID_PARAMETER;
    }

    AFD_DbgPrint(MID_TRACE,("Called(TransportObject %x)\n", TransportObject));

    DeviceObject = IoGetRelatedDeviceObject(TransportObject);
    if (!DeviceObject) {
        AFD_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    Irp = TdiBuildInternalDeviceControlIrp
	( TDI_SEND_DATAGRAM,       /* Sub function */
	  DeviceObject,            /* Device object */
	  TransportObject,         /* File object */
	  &Event,                  /* Event */
	  Iosb );                  /* Status */

    if (!Irp) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TdiBuildDisconnect
	(Irp,                    /* I/O Request Packet */
	 DeviceObject,           /* Device object */
	 TransportObject,        /* File object */
	 CompletionRoutine,      /* Completion routine */
	 CompletionContext,      /* Completion context */
	 Time,                   /* Time */
	 Flags,                  /* Disconnect flags */
	 RequestConnectionInfo,  /* Indication of who to disconnect */
	 ReturnConnectionInfo);  /* Indication of who disconnected */

    Status = TdiCall(Irp, DeviceObject, &Event, Iosb);

    return Status;
}

/* EOF */
