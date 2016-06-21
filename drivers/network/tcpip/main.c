/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/main.c
 * PURPOSE:         tcpip.sys driver entry
 */

#include "precomp.h"

//#define NDEBUG
#include <debug.h>

/* DriverEntry, DriverUnload and dispatch routines declaration */
DRIVER_INITIALIZE DriverEntry;
static DRIVER_UNLOAD TcpIpUnload;
static DRIVER_DISPATCH TcpIpCreate;
static DRIVER_DISPATCH TcpIpClose;
static DRIVER_DISPATCH TcpIpDispatchInternal;
static DRIVER_DISPATCH TcpIpDispatch;
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, TcpIpUnload)
#pragma alloc_text(PAGE, TcpIpCreate)
#pragma alloc_text(PAGE, TcpIpClose)
#pragma alloc_text(PAGE, TcpIpDispatchInternal)
#pragma alloc_text(PAGE, TcpIpDispatch)
#endif

/* Our device objects. TCP, UPD, IP, and RAW */
PDEVICE_OBJECT TcpDeviceObject   = NULL;
PDEVICE_OBJECT UdpDeviceObject   = NULL;
PDEVICE_OBJECT IpDeviceObject    = NULL;
PDEVICE_OBJECT RawIpDeviceObject = NULL;

/* And the corresponding device names */
#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_IP_DEVICE_NAME       L"\\Device\\Ip"
#define DD_RAWIP_DEVICE_NAME    L"\\Device\\RawIp"

/* This is a small utility which get the IPPROTO_* constant from the device object this driver was passed */
static
IPPROTO
ProtocolFromIrp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    UNICODE_STRING ProtocolName;
    PWCHAR Name;
    ULONG Value;
    NTSTATUS Status;

    if (DeviceObject == TcpDeviceObject)
        return IPPROTO_TCP;
    if (DeviceObject == UdpDeviceObject)
        return IPPROTO_UDP;
    if (DeviceObject == IpDeviceObject)
        return IPPROTO_RAW;

    /* Get it from the IRP file object */
    Name = IrpSp->FileObject->FileName.Buffer;

    if (*Name++ != L'\\')
    {
        DPRINT1("Could not deduce protocol from file name %wZ.\n", &IrpSp->FileObject->FileName);
        return (IPPROTO)-1;
    }

    RtlInitUnicodeString(&ProtocolName, Name);
    Status = RtlUnicodeStringToInteger(&ProtocolName, 10, &Value);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not deduce protocol from file name %wZ.\n", &IrpSp->FileObject->FileName);
        return (IPPROTO)-1;
    }

    if (Value >= IPPROTO_RESERVED_MAX)
    {
        DPRINT1("Could not deduce protocol from file name %wZ.\n", &IrpSp->FileObject->FileName);
        return (IPPROTO)-1;
    }

    return (IPPROTO)Value;
}

NTSTATUS
NTAPI
DriverEntry(
  _In_  struct _DRIVER_OBJECT *DriverObject,
  _In_  PUNICODE_STRING RegistryPath
)
{
    UNICODE_STRING IpDeviceName    = RTL_CONSTANT_STRING(DD_IP_DEVICE_NAME);
    UNICODE_STRING RawIpDeviceName = RTL_CONSTANT_STRING(DD_RAWIP_DEVICE_NAME);
    UNICODE_STRING UdpDeviceName   = RTL_CONSTANT_STRING(DD_UDP_DEVICE_NAME);
    UNICODE_STRING TcpDeviceName   = RTL_CONSTANT_STRING(DD_TCP_DEVICE_NAME);
    NTSTATUS Status;

    /* Initialize the lwip library */
    tcpip_init(NULL, NULL);

    /* Create the device objects */
    Status = IoCreateDevice(
        DriverObject,
        0,
        &IpDeviceName,
        FILE_DEVICE_NETWORK,
        0,
        FALSE,
        &IpDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create device object. Status: 0x%08x\n", Status);
        goto Failure;
    }

    Status = IoCreateDevice(
        DriverObject,
        0,
        &UdpDeviceName,
        FILE_DEVICE_NETWORK,
        0,
        FALSE,
        &UdpDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create device object. Status: 0x%08x\n", Status);
        goto Failure;
    }

    Status = IoCreateDevice(
        DriverObject,
        0,
        &TcpDeviceName,
        FILE_DEVICE_NETWORK,
        0,
        FALSE,
        &TcpDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create device object. Status: 0x%08x\n", Status);
        goto Failure;
    }

    Status = IoCreateDevice(
        DriverObject,
        0,
        &RawIpDeviceName,
        FILE_DEVICE_NETWORK,
        0,
        FALSE,
        &RawIpDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create device object. Status: 0x%08x\n", Status);
        goto Failure;
    }

    /* Use direct I/O with this devices */
    IpDeviceObject->Flags    |= DO_DIRECT_IO;
    RawIpDeviceObject->Flags |= DO_DIRECT_IO;
    UdpDeviceObject->Flags   |= DO_DIRECT_IO;
    TcpDeviceObject->Flags   |= DO_DIRECT_IO;

    /* Set driver object entry points */
    DriverObject->MajorFunction[IRP_MJ_CREATE]  = TcpIpCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]   = TcpIpClose;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = TcpIpDispatchInternal;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TcpIpDispatch;
    DriverObject->DriverUnload = TcpIpUnload;

    /* Initialize various parts of the driver */
    TcpIpInitializeAddresses();
    TcpIpInitializeTcp();
    TcpIpInitializeEntities();
    Status = TcpIpRegisterNdisProtocol();
    if (!NT_SUCCESS(Status))
        goto Failure;

    /* Create the loopback interface */
    Status = TcpIpCreateLoopbackInterface();
    if (!NT_SUCCESS(Status))
        goto Failure;

    return STATUS_SUCCESS;

Failure:
    TcpIpUnregisterNdisProtocol();

    if (IpDeviceObject)
        IoDeleteDevice(IpDeviceObject);
    if (TcpDeviceObject)
        IoDeleteDevice(TcpDeviceObject);
    if (UdpDeviceObject)
        IoDeleteDevice(UdpDeviceObject);
    if (RawIpDeviceObject)
        IoDeleteDevice(RawIpDeviceObject);

    return Status;
}

static
NTSTATUS
NTAPI
TcpIpCreate(
    _Inout_  struct _DEVICE_OBJECT *DeviceObject,
    _Inout_  struct _IRP *Irp
)
{
    NTSTATUS Status;
    PFILE_FULL_EA_INFORMATION FileInfo;
    IPPROTO Protocol;
//	ADDRESS_FILE *AddressFile;
	
//	ULONG *temp;
	
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
    /* Grab the info describing the file */
    FileInfo = Irp->AssociatedIrp.SystemBuffer;

    if (!FileInfo)
    {
        /* Caller just wants a control channel. We don't need any structure for this kind of "file" */
        IrpSp->FileObject->FsContext = NULL;
        IrpSp->FileObject->FsContext2 = (PVOID)TDI_CONTROL_CHANNEL_FILE;
        Status = STATUS_SUCCESS;
        goto Quickie;
    }
	
    /* Validate it */
    switch (FileInfo->EaNameLength)
    {
        case TDI_TRANSPORT_ADDRESS_LENGTH:
        {
            PTA_IP_ADDRESS Address;
			
			DPRINT1("TCPIP Create Transport Address\n");

            if (strncmp(&FileInfo->EaName[0], TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH) != 0)
            {
                DPRINT1("TCPIP: Should maybe open file %*s.\n", FileInfo->EaNameLength, &FileInfo->EaName[0]);
                Status = STATUS_INVALID_PARAMETER;
                goto Quickie;
            }
            /* Get the address info right after the file name info */
            Address = (PTA_IP_ADDRESS)(&FileInfo->EaName[FileInfo->EaNameLength + 1]);

            /* Validate it */
            if ((FileInfo->EaValueLength < sizeof(*Address)) ||
                    (Address->TAAddressCount != 1) ||
                    (Address->Address[0].AddressLength < TDI_ADDRESS_LENGTH_IP) ||
                    (Address->Address[0].AddressType != TDI_ADDRESS_TYPE_IP))
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Quickie;
            }

            /* Get the protocol this address will be created for. */
            Protocol = ProtocolFromIrp(DeviceObject, IrpSp);
            if (Protocol == (IPPROTO)-1)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Quickie;
            }

            /* All good. */
/*			temp = (ULONG*)Address;
			DPRINT1("\nPTA_IP_ADDRESS dump before\n %08x %08x %08x %08x\n %08x %08x %08x %08x\n",
				temp[7], temp[6], temp[5], temp[4],
				temp[3], temp[2], temp[1], temp[0]);*/
			Status = TcpIpCreateAddress(Irp, &Address->Address[0].Address[0], Protocol);
			if (Status != STATUS_SUCCESS)
			{
				goto Quickie;
			}
/*			DPRINT1("\nPTA_IP_ADDRESS dump after\n %08x %08x %08x %08x\n %08x %08x %08x %08x\n",
				temp[7], temp[6], temp[5], temp[4],
				temp[3], temp[2], temp[1], temp[0]);*/
            break;
        }

        case TDI_CONNECTION_CONTEXT_LENGTH:
		{
			PTA_IP_ADDRESS Address;
			
			DPRINT1("TCPIP Create connection Context\n");
			
            if (strncmp(&FileInfo->EaName[0], TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH) != 0)
            {
                DPRINT1("TCPIP: Should maybe open file %*s.\n", FileInfo->EaNameLength, &FileInfo->EaName[0]);
                return STATUS_INVALID_PARAMETER;
            }
			
			Address = (PTA_IP_ADDRESS)(&FileInfo->EaName[FileInfo->EaNameLength + 1]);
			
			/* Get the protocol this address will be created for. */
            Protocol = ProtocolFromIrp(DeviceObject, IrpSp);
			if (Protocol == (IPPROTO)-1)
			{
				Status = STATUS_INVALID_PARAMETER;
				goto Quickie;
			}
			
/*			temp = (ULONG*)Protocol;
			DPRINT1("\n Protocol: %08x\n", temp);
			
			temp = (ULONG*)Address;*/
			
			/* All good. */
/*			DPRINT1("\n PTA_IP_ADDRESS dump before\n  %08x %08x %08x %08x\n  %08x %08x %08x %08x\n",
				temp[7], temp[6], temp[5], temp[4],
				temp[3], temp[2], temp[1], temp[0]);*/
			Status = TcpIpCreateContext(Irp, &Address->Address[0].Address[0], Protocol);
/*			DPRINT1("\n PTA_IP_ADDRESS dump after\n  %08x %08x %08x %08x\n  %08x %08x %08x %08x\n",
				temp[7], temp[6], temp[5], temp[4],
				temp[3], temp[2], temp[1], temp[0]);*/
            break;
		}

        default:
            DPRINT1("TCPIP: Should open file %*s.\n", FileInfo->EaNameLength, &FileInfo->EaName[0]);
            Status = STATUS_INVALID_PARAMETER;
    }

Quickie:
    Irp->IoStatus.Status = Status;
    if (Status == STATUS_PENDING)
    {
        IoMarkIrpPending(Irp);
    }
    else
    {
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    return Status;
}

static
NTSTATUS
NTAPI
TcpIpClose(
    _Inout_  struct _DEVICE_OBJECT *DeviceObject,
    _Inout_  struct _IRP *Irp
)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    ULONG_PTR FileType;
	
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    FileType = (ULONG_PTR)IrpSp->FileObject->FsContext2;

    switch (FileType)
    {
        case TDI_TRANSPORT_ADDRESS_FILE:
			DPRINT1("TCPIP Close Transport Address\n");
		
            if (!IrpSp->FileObject->FsContext)
            {
                DPRINT1("TCPIP: Got a close request without a file to close!\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quickie;
            }
            Status = TcpIpCloseAddress(IrpSp->FileObject->FsContext);
            break;
        case TDI_CONTROL_CHANNEL_FILE:
            /* We didn't allocate anything for this. */
            Status = STATUS_SUCCESS;
            break;
        default:
            DPRINT1("TCPIP: Should close file %Iu.\n", FileType);
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

Quickie:
    Irp->IoStatus.Status = Status;

	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	return Status;
}

static
NTSTATUS
NTAPI
TcpIpDispatchInternal(
    _Inout_  struct _DEVICE_OBJECT *DeviceObject,
    _Inout_  struct _IRP *Irp
)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
	PADDRESS_FILE AddressFile;

	DPRINT1("TcpIpDispatchInternal\n");
	
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

	AddressFile = IrpSp->FileObject->FsContext;
	
    switch (IrpSp->MinorFunction)
    {
        case TDI_RECEIVE:
            DPRINT1("TCPIP: TDI_RECEIVE!\n");
			switch (AddressFile->Protocol)
			{
				case IPPROTO_TCP:
					Status = TcpIpReceive(Irp);
					break;
				case IPPROTO_UDP:
					Status = STATUS_NOT_IMPLEMENTED;
					break;
				case IPPROTO_RAW:
					Status = STATUS_NOT_IMPLEMENTED;
					break;
				default:
					Status = STATUS_NOT_IMPLEMENTED;
			}
            if (Status == STATUS_NOT_IMPLEMENTED)
			{
				DPRINT1("Received TDI_RECEIVE for non-TCP protocol\n");
				
			}
			break;
        case TDI_RECEIVE_DATAGRAM:
			DPRINT1("TCPIP: TDI_RECEIVE_DATAGRAM!\n");
            return TcpIpReceiveDatagram(Irp);

        case TDI_SEND:
            DPRINT1("TCPIP: TDI_SEND!\n");
			switch (AddressFile->Protocol)
			{
				case IPPROTO_TCP:
					Status = TcpIpSend(Irp);
					break;
				case IPPROTO_UDP:
					Status = STATUS_NOT_IMPLEMENTED;
					break;
				case IPPROTO_RAW:
					Status = STATUS_NOT_IMPLEMENTED;
					break;
				default:
					Status = STATUS_NOT_IMPLEMENTED;
			}
            if (Status == STATUS_NOT_IMPLEMENTED)
			{
				DPRINT1("Received TDI_SEND for non-TCP protocol\n");
			}
            break;

        case TDI_SEND_DATAGRAM:
			DPRINT1("TCPIP: TDI_SEND_DATAGRAM!\n");
            return TcpIpSendDatagram(Irp);

        case TDI_ACCEPT:
            DPRINT1("TCPIP: TDI_ACCEPT!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case TDI_LISTEN:
			DPRINT1("TCPIP: TDI_LISTEN!\n");
            Status = TcpIpListen(Irp);
			break;

        case TDI_CONNECT:
            DPRINT1("TCPIP: TDI_CONNECT!\n");
            Status = TcpIpConnect(Irp);
			break;

        case TDI_DISCONNECT:
            DPRINT1("TCPIP: TDI_DISCONNECT!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case TDI_ASSOCIATE_ADDRESS:
			DPRINT1("TCPIP: TDI_ASSOCIATE_ADDRESS\n");
            Status = TcpIpAssociateAddress(Irp);
			break;

        case TDI_DISASSOCIATE_ADDRESS:
            DPRINT1("TCPIP: TDI_DISASSOCIATE_ADDRESS!\n");
            Status = TcpIpDisassociateAddress(Irp);
            break;

        case TDI_QUERY_INFORMATION:
			DPRINT1("TCPIP: TDI_QUERY_INFORMATION\n");
            return TcpIpQueryKernelInformation(Irp);

        case TDI_SET_INFORMATION:
            DPRINT1("TCPIP: TDI_SET_INFORMATION!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case TDI_SET_EVENT_HANDLER:
            DPRINT1("TCPIP: TDI_SET_EVENT_HANDLER!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case TDI_ACTION:
            DPRINT1("TDI_ACTION!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* An unsupported IOCTL code was submitted */
        default:
            DPRINT1("TCPIP: Unknown internal IOCTL: 0x%x.\n", IrpSp->MinorFunction);
            Status = STATUS_NOT_IMPLEMENTED;
    }

    Irp->IoStatus.Status = Status;
    if (Status == STATUS_PENDING)
    {
        IoMarkIrpPending(Irp);
    }
    else
    {
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    return Status;
}

static
NTSTATUS
NTAPI
TcpIpDispatch(
    _Inout_  struct _DEVICE_OBJECT *DeviceObject,
    _Inout_  struct _IRP *Irp
)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
	
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Information = 0;

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_TCP_QUERY_INFORMATION_EX:
            Status = TcpIpQueryInformation(Irp);
			break;

		case IOCTL_TCP_SET_INFORMATION_EX:
			Status = TcpIpSetInformation(Irp);
			break;

		case IOCTL_SET_IP_ADDRESS:
			DPRINT1("TCPIP: Should handle IOCTL_SET_IP_ADDRESS.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;

		case IOCTL_DELETE_IP_ADDRESS:
			DPRINT1("TCPIP: Should handle IOCTL_DELETE_IP_ADDRESS.\n");
			Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DPRINT1("TCPIP: Unknown IOCTL 0x%#x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
			Status = STATUS_NOT_IMPLEMENTED;
			break;
    }

    //DPRINT("TCPIP dispatched with status 0x%08x.\n", Status);

    Irp->IoStatus.Status = Status;
    if (Status == STATUS_PENDING)
    {
        IoMarkIrpPending(Irp);
    }
    else
    {
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    return Status;
}

static
VOID
NTAPI
TcpIpUnload(
    _In_  struct _DRIVER_OBJECT *DriverObject
)
{
    IoDeleteDevice(IpDeviceObject);
    IoDeleteDevice(RawIpDeviceObject);
    IoDeleteDevice(UdpDeviceObject);
    IoDeleteDevice(TcpDeviceObject);
    TcpIpUnregisterNdisProtocol();
}
