/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/main.c
 * PURPOSE:     Driver entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <dispatch.h>
#include <fileobjs.h>
#include <datagram.h>
#include <loopback.h>
#include <rawip.h>
#include <udp.h>
#include <tcp.h>


#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

PDEVICE_OBJECT TCPDeviceObject   = NULL;
PDEVICE_OBJECT UDPDeviceObject   = NULL;
PDEVICE_OBJECT IPDeviceObject    = NULL;
PDEVICE_OBJECT RawIPDeviceObject = NULL;
NDIS_HANDLE GlobalPacketPool     = NULL;
NDIS_HANDLE GlobalBufferPool     = NULL;
TDIEntityID *EntityList          = NULL;
ULONG EntityCount                = 0;
UDP_STATISTICS UDPStats;


VOID TiWriteErrorLog(
    PDRIVER_OBJECT DriverContext,
    NTSTATUS ErrorCode,
    ULONG UniqueErrorValue,
    NTSTATUS FinalStatus,
    PWSTR String,
    ULONG DumpDataCount,
    PULONG DumpData)
/*
 * FUNCTION: Writes an error log entry
 * ARGUMENTS:
 *     DriverContext    = Pointer to the driver or device object
 *     ErrorCode        = An error code to put in the log entry
 *     UniqueErrorValue = UniqueErrorValue in the error log packet
 *     FinalStatus      = FinalStatus in the error log packet
 *     String           = If not NULL, a pointer to a string to put in log entry
 *     DumpDataCount    = Number of ULONGs of dump data
 *     DumpData         = Pointer to dump data for the log entry
 */
{
#ifdef _MSC_VER
    PIO_ERROR_LOG_PACKET LogEntry;
    UCHAR EntrySize;
    ULONG StringSize;
    PUCHAR pString;
    static WCHAR DriverName[] = L"TCP/IP";

    EntrySize = sizeof(IO_ERROR_LOG_PACKET) + 
        (DumpDataCount * sizeof(ULONG)) + sizeof(DriverName);

    if (String) {
        StringSize = (wcslen(String) * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
        EntrySize += (UCHAR)StringSize;
    }

    LogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
		DriverContext, EntrySize);

    if (LogEntry) {
        LogEntry->MajorFunctionCode = -1;
        LogEntry->RetryCount        = -1;
        LogEntry->DumpDataSize      = (USHORT)(DumpDataCount * sizeof(ULONG));
        LogEntry->NumberOfStrings   = (String == NULL) ? 1 : 2;
        LogEntry->StringOffset      = sizeof(IO_ERROR_LOG_PACKET) + (DumpDataCount-1) * sizeof(ULONG);
        LogEntry->EventCategory     = 0;
        LogEntry->ErrorCode         = ErrorCode;
        LogEntry->UniqueErrorValue  = UniqueErrorValue;
        LogEntry->FinalStatus       = FinalStatus;
        LogEntry->SequenceNumber    = -1;
        LogEntry->IoControlCode     = 0;

        if (DumpDataCount)
            RtlCopyMemory(LogEntry->DumpData, DumpData, DumpDataCount * sizeof(ULONG));

        pString = ((PUCHAR)LogEntry) + LogEntry->StringOffset;
        RtlCopyMemory(pString, DriverName, sizeof(DriverName));
        pString += sizeof(DriverName);

        if (String)
            RtlCopyMemory(pString, String, StringSize);

        IoWriteErrorLogEntry(LogEntry);
    }
#endif
}


/*
 * FUNCTION: Creates a file object
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
NTSTATUS TiCreateFileObject(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    PFILE_FULL_EA_INFORMATION EaInfo;
    PTA_ADDRESS_IP Address;
    PTRANSPORT_CONTEXT Context;
    TDI_REQUEST Request;
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_IRP, ("Called. DeviceObject is at (0x%X), IRP is at (0x%X).\n", DeviceObject, Irp));

    EaInfo = Irp->AssociatedIrp.SystemBuffer;

    /* Parameter check */
    if (!EaInfo) {
        TI_DbgPrint(MIN_TRACE, ("No EA information in IRP.\n"));
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Allocate resources here. We release them again if something failed */
    Context = ExAllocatePool(NonPagedPool, sizeof(TRANSPORT_CONTEXT));
    if (!Context) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Context->RefCount   = 1;
    Context->CancelIrps = FALSE;
    KeInitializeEvent(&Context->CleanupEvent, NotificationEvent, FALSE);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    IrpSp->FileObject->FsContext = Context;
    Request.RequestContext       = Irp;

    /* Branch to the right handler */
    if ((EaInfo->EaNameLength==TDI_TRANSPORT_ADDRESS_LENGTH) && 
        (RtlCompareMemory(&EaInfo->EaName, TdiTransportAddress,
         TDI_TRANSPORT_ADDRESS_LENGTH) == TDI_TRANSPORT_ADDRESS_LENGTH)) {
        /* This is a request to open an address */

        /* Parameter checks */
        Address = (PTA_ADDRESS_IP)(EaInfo->EaName + EaInfo->EaNameLength);
        if ((EaInfo->EaValueLength < sizeof(TA_ADDRESS_IP)) ||
            (Address->TAAddressCount != 1) ||
            (Address->Address[0].AddressLength < TDI_ADDRESS_LENGTH_IP) ||
            (Address->Address[0].AddressType != TDI_ADDRESS_TYPE_IP)) {
            TI_DbgPrint(MIN_TRACE, ("Parameters are invalid.\n"));
            ExFreePool(Context);
            return STATUS_INVALID_PARAMETER;
        }

        /* Open address file object */
        /* FIXME: Protocol depends on device object */
        Status = FileOpenAddress(&Request, Address, IPPROTO_UDP, NULL);
        if (NT_SUCCESS(Status)) {
            IrpSp->FileObject->FsContext2 = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
            Context->Handle.AddressHandle = Request.Handle.AddressHandle;
        }
    } else {
        TI_DbgPrint(MIN_TRACE, ("Connection point, and control connections are not supported.\n"));
        /* FIXME: Open a connection endpoint, or control connection */
        Status = STATUS_NOT_IMPLEMENTED;
    }

    if (!NT_SUCCESS(Status))
        ExFreePool(Context);

    TI_DbgPrint(DEBUG_IRP, ("Leaving. Status = (0x%X).\n", Status));

    return Status;
}


VOID TiCleanupFileObjectComplete(
    PVOID Context,
    NTSTATUS Status)
/*
 * FUNCTION: Completes an object cleanup IRP I/O request
 * ARGUMENTS:
 *     Context = Pointer to the IRP for this request
 *     Status  = Final status of the operation
 */
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT TranContext;
    KIRQL OldIrql;

    Irp         = (PIRP)Context;
    IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;

    Irp->IoStatus.Status = Status;
    
    IoAcquireCancelSpinLock(&OldIrql);

    /* Remove the initial reference provided at object creation time */
    TranContext->RefCount--;

#ifdef DBG
    if (TranContext->RefCount != 0)
        TI_DbgPrint(DEBUG_REFCOUNT, ("TranContext->RefCount is %i, should be 0.\n", TranContext->RefCount));
#endif

    KeSetEvent(&TranContext->CleanupEvent, 0, FALSE);

    IoReleaseCancelSpinLock(OldIrql);
}


/*
 * FUNCTION: Releases resources used by a file object
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 * NOTES:
 *     This function does not pend
 */
NTSTATUS TiCleanupFileObject(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT Context;
    TDI_REQUEST Request;
    NTSTATUS Status;
    KIRQL OldIrql;

    IrpSp   = IoGetCurrentIrpStackLocation(Irp);
    Context = IrpSp->FileObject->FsContext;    
    if (!Context) {
        TI_DbgPrint(MIN_TRACE, ("Parameters are invalid.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    IoAcquireCancelSpinLock(&OldIrql);

    Context->CancelIrps = TRUE;
    KeResetEvent(&Context->CleanupEvent);

    IoReleaseCancelSpinLock(OldIrql);

    Request.RequestNotifyObject = TiCleanupFileObjectComplete;
    Request.RequestContext      = Irp;

    switch ((ULONG_PTR)IrpSp->FileObject->FsContext2) {
    case TDI_TRANSPORT_ADDRESS_FILE:
        Request.Handle.AddressHandle = Context->Handle.AddressHandle;
        Status = FileCloseAddress(&Request);
        break;

    case TDI_CONNECTION_FILE:
        Request.Handle.ConnectionContext = Context->Handle.ConnectionContext;
        Status = FileCloseConnection(&Request);
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        Request.Handle.ControlChannel = Context->Handle.ControlChannel;
        Status = FileCloseControlChannel(&Request);
        break;

    default:
        /* This should never happen */

        TI_DbgPrint(MIN_TRACE, ("Unknown transport context.\n"));

        IoAcquireCancelSpinLock(&OldIrql);
        Context->CancelIrps = FALSE;
        IoReleaseCancelSpinLock(OldIrql);

        return STATUS_INVALID_PARAMETER;
    }

    if (Status != STATUS_PENDING)
       TiCleanupFileObjectComplete(Irp, Status);
    
    KeWaitForSingleObject(&Context->CleanupEvent,
        UserRequest, KernelMode, FALSE, NULL);
    
    return Irp->IoStatus.Status;
}


NTSTATUS TiDispatchOpenClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Main dispath routine
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    PTRANSPORT_CONTEXT Context;

    TI_DbgPrint(DEBUG_IRP, ("Called. DeviceObject is at (0x%X), IRP is at (0x%X).\n", DeviceObject, Irp));

    IoMarkIrpPending(Irp);
    Irp->IoStatus.Status      = STATUS_PENDING;
    Irp->IoStatus.Information = 0;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MajorFunction) {
    /* Open an address file, connection endpoint, or control connection */
    case IRP_MJ_CREATE:
        Status = TiCreateFileObject(DeviceObject, Irp);
        break;

    /* Close an address file, connection endpoint, or control connection */
    case IRP_MJ_CLOSE:
        Context = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;
        if (Context)
            ExFreePool(Context);
        Status = STATUS_SUCCESS;
        break;

    /* Release resources bound to an address file, connection endpoint, 
       or control connection */
    case IRP_MJ_CLEANUP:
        Status = TiCleanupFileObject(DeviceObject, Irp);
        break;

	default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    if (Status != STATUS_PENDING) {
        IrpSp->Control &= ~SL_PENDING_RETURNED;
        Irp->IoStatus.Status = Status;

        TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    TI_DbgPrint(DEBUG_IRP, ("Leaving. Status is (0x%X)\n", Status));

    return Status;
}


NTSTATUS TiDispatchInternal(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Internal IOCTL dispatch routine
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
{
	NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    TI_DbgPrint(DEBUG_IRP, ("Called. DeviceObject is at (0x%X), IRP is at (0x%X) MN (%d).\n",
        DeviceObject, Irp, IrpSp->MinorFunction));

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    switch (IrpSp->MinorFunction) {
    case TDI_RECEIVE:
        Status = DispTdiReceive(Irp);
        break;

    case TDI_RECEIVE_DATAGRAM:
        Status = DispTdiReceiveDatagram(Irp);
        break;

    case TDI_SEND:
        Status = DispTdiSend(Irp);
        break;

    case TDI_SEND_DATAGRAM:
        Status = DispTdiSendDatagram(Irp);
        break;

    case TDI_ACCEPT:
        Status = DispTdiAccept(Irp);
        break;

    case TDI_LISTEN:
        Status = DispTdiListen(Irp);
        break;

    case TDI_CONNECT:
        Status = DispTdiConnect(Irp);
        break;

    case TDI_DISCONNECT:
        Status = DispTdiDisconnect(Irp);
        break;

    case TDI_ASSOCIATE_ADDRESS:
        Status = DispTdiAssociateAddress(Irp);
        break;

    case TDI_DISASSOCIATE_ADDRESS:
        Status = DispTdiDisassociateAddress(Irp);
        break;

    case TDI_QUERY_INFORMATION:
        Status = DispTdiQueryInformation(DeviceObject, Irp);
        break;

    case TDI_SET_INFORMATION:
        Status = DispTdiSetInformation(Irp);
        break;

    case TDI_SET_EVENT_HANDLER:
        Status = DispTdiSetEventHandler(Irp);
        break;

    case TDI_ACTION:
        Status = STATUS_SUCCESS;
        break;

    /* An unsupported IOCTL code was submitted */
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;

        TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    TI_DbgPrint(DEBUG_IRP, ("Leaving. Status = (0x%X).\n", Status));

	return Status;
}


NTSTATUS TiDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Dispath routine for IRP_MJ_DEVICE_CONTROL requests
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    TI_DbgPrint(DEBUG_IRP, ("Called. IRP is at (0x%X).\n", Irp));

    Irp->IoStatus.Information = 0;

    IrpSp  = IoGetCurrentIrpStackLocation(Irp);
#ifdef _MSC_VER
    Status = TdiMapUserRequest(DeviceObject, Irp, IrpSp);
    if (NT_SUCCESS(Status)) {
        TiDispatchInternal(DeviceObject, Irp);
        Status = STATUS_PENDING;
    } else {
#else
    if (TRUE) {
#endif
        /* See if this request is TCP/IP specific */
        switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_TCP_QUERY_INFORMATION_EX:
            Status = DispTdiQueryInformationEx(Irp, IrpSp);
            break;

        case IOCTL_TCP_SET_INFORMATION_EX:
            Status = DispTdiSetInformationEx(Irp, IrpSp);
            break;

        default:
            TI_DbgPrint(MIN_TRACE, ("Unknown IOCTL 0x%X\n",
                IrpSp->Parameters.DeviceIoControl.IoControlCode));
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;

        TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    TI_DbgPrint(DEBUG_IRP, ("Leaving. Status = (0x%X).\n", Status));

    return Status;
}


VOID TiUnload(
    PDRIVER_OBJECT DriverObject)
/*
 * FUNCTION: Unloads the driver
 * ARGUMENTS:
 *     DriverObject = Pointer to driver object created by the system
 */
{
#ifdef DBG
    KIRQL OldIrql;

    KeAcquireSpinLock(&AddressFileListLock, &OldIrql);
    if (!IsListEmpty(&AddressFileListHead)) {
        TI_DbgPrint(MIN_TRACE, ("Open address file objects exists.\n"));
    }
    KeReleaseSpinLock(&AddressFileListLock, OldIrql);
#endif

    /* Unregister loopback adapter */
    LoopUnregisterAdapter(NULL);

    /* Unregister protocol with NDIS */
    LANUnregisterProtocol();

    /* Shutdown transport level protocol subsystems */
    TCPShutdown();
    UDPShutdown();
    RawIPShutdown();
    DGShutdown();

    /* Shutdown network level protocol subsystem */
    IPShutdown();

    /* Free NDIS buffer descriptors */
    if (GlobalBufferPool)
        NdisFreeBufferPool(GlobalBufferPool);

    /* Free NDIS packet descriptors */
    if (GlobalPacketPool)
        NdisFreePacketPool(GlobalPacketPool);

    /* Release all device objects */

    if (TCPDeviceObject)
        IoDeleteDevice(TCPDeviceObject);

    if (UDPDeviceObject)
        IoDeleteDevice(UDPDeviceObject);

    if (RawIPDeviceObject)
        IoDeleteDevice(RawIPDeviceObject);

    if (IPDeviceObject)
        IoDeleteDevice(IPDeviceObject);

    if (EntityList)
        ExFreePool(EntityList);

    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS
#ifndef _MSC_VER
STDCALL
#endif
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Main driver entry point
 * ARGUMENTS:
 *     DriverObject = Pointer to a driver object for this driver
 *     RegistryPath = Registry node for configuration parameters
 * RETURNS:
 *     Status of driver initialization
 */
{
    NTSTATUS Status;
    UNICODE_STRING strDeviceName;
    STRING strNdisDeviceName;
    NDIS_STATUS NdisStatus;
    PLAN_ADAPTER Adapter;
    NDIS_STRING DeviceName;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Create symbolic links in Win32 namespace */

    /* Create IP device object */
    RtlInitUnicodeString(&strDeviceName, DD_IP_DEVICE_NAME);
    Status = IoCreateDevice(DriverObject, 0, &strDeviceName,
        FILE_DEVICE_NETWORK, 0, FALSE, &IPDeviceObject);
    if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(MIN_TRACE, ("Failed to create IP device object. Status (0x%X).\n", Status));
        return Status;
    }

    /* Create RawIP device object */
    RtlInitUnicodeString(&strDeviceName, DD_RAWIP_DEVICE_NAME);
    Status = IoCreateDevice(DriverObject, 0, &strDeviceName,
        FILE_DEVICE_NETWORK, 0, FALSE, &RawIPDeviceObject);
    if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(MIN_TRACE, ("Failed to create RawIP device object. Status (0x%X).\n", Status));
        TiUnload(DriverObject);
        return Status;
    }

    /* Create UDP device object */
    RtlInitUnicodeString(&strDeviceName, DD_UDP_DEVICE_NAME);
    Status = IoCreateDevice(DriverObject, 0, &strDeviceName,
        FILE_DEVICE_NETWORK, 0, FALSE, &UDPDeviceObject);
    if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(MIN_TRACE, ("Failed to create UDP device object. Status (0x%X).\n", Status));
        TiUnload(DriverObject);
        return Status;
    }

    /* Create TCP device object */
    RtlInitUnicodeString(&strDeviceName, DD_TCP_DEVICE_NAME);
    Status = IoCreateDevice(DriverObject, 0, &strDeviceName,
        FILE_DEVICE_NETWORK, 0, FALSE, &TCPDeviceObject);
    if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(MIN_TRACE, ("Failed to create TCP device object. Status (0x%X).\n", Status));
        TiUnload(DriverObject);
        return Status;
    }

    /* Allocate NDIS packet descriptors */
    NdisAllocatePacketPool(&NdisStatus, &GlobalPacketPool, 100, sizeof(PACKET_CONTEXT));
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TiUnload(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate NDIS buffer descriptors */
    NdisAllocateBufferPool(&NdisStatus, &GlobalBufferPool, 100);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        TiUnload(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize address file list and protecting spin lock */
    InitializeListHead(&AddressFileListHead);
    KeInitializeSpinLock(&AddressFileListLock);

    /* Initialize interface list and protecting spin lock */
    InitializeListHead(&InterfaceListHead);
    KeInitializeSpinLock(&InterfaceListLock);

    /* Initialize network level protocol subsystem */
    IPStartup(DriverObject, RegistryPath);

    /* Initialize transport level protocol subsystems */
    DGStartup();
    RawIPStartup();
    UDPStartup();
    TCPStartup();

    /* Register protocol with NDIS */
    RtlInitString(&strNdisDeviceName, IP_DEVICE_NAME);
    Status = LANRegisterProtocol(&strNdisDeviceName);
    if (!NT_SUCCESS(Status)) {
		TiWriteErrorLog(
            DriverObject,
            EVENT_TRANSPORT_REGISTER_FAILED,
            TI_ERROR_DRIVERENTRY,
            Status,
            NULL,
            0,
            NULL);
        TiUnload(DriverObject);
        return Status;
    }

    /* Open loopback adapter */
    if (!NT_SUCCESS(LoopRegisterAdapter(NULL, NULL))) {
        TI_DbgPrint(MIN_TRACE, ("Failed to create loopback adapter. Status (0x%X).\n", Status));
        TiUnload(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if 0
    /* Open underlying adapter(s) we are bound to */

    /* FIXME: Get binding information from registry */

    /* Put your own NDIS adapter device name here */

    /* ReactOS */
    NdisInitUnicodeString(&DeviceName, L"\\Device\\ne2000");

    /* NT4 */
    //NdisInitUnicodeString(&DeviceName, L"\\Device\\El90x1");

    /* NT5 */
    //NdisInitUnicodeString(&DeviceName, L"\\Device\\{56388B49-67BB-4419-A3F4-28DF190B9149}");

    NdisStatus = LANRegisterAdapter(&DeviceName, &Adapter);
    if (!NT_SUCCESS(NdisStatus)) {
        TI_DbgPrint(MIN_TRACE, ("Failed to intialize adapter. Status (0x%X).\n", Status));
		TiWriteErrorLog(
            DriverObject,
            EVENT_TRANSPORT_ADAPTER_NOT_FOUND,
            TI_ERROR_DRIVERENTRY,
            NdisStatus,
            NULL,
            0,
            NULL);
        TiUnload(DriverObject);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }
#endif

    /* Setup network layer and transport layer entities */
    EntityList = ExAllocatePool(NonPagedPool, sizeof(TDIEntityID) * 2);
    if (!NT_SUCCESS(Status)) {
        TiUnload(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    EntityList[0].tei_entity   = CL_NL_ENTITY;
    EntityList[0].tei_instance = 0;
    EntityList[1].tei_entity   = CL_TL_ENTITY;
    EntityList[1].tei_instance = 0;
    EntityCount = 2;

    /* Use direct I/O */
    IPDeviceObject->Flags    |= DO_DIRECT_IO;
    RawIPDeviceObject->Flags |= DO_DIRECT_IO;
    UDPDeviceObject->Flags   |= DO_DIRECT_IO;
    TCPDeviceObject->Flags   |= DO_DIRECT_IO;

    /* Initialize the driver object with this driver's entry points */
    DriverObject->MajorFunction[IRP_MJ_CREATE]  = TiDispatchOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]   = TiDispatchOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = TiDispatchOpenClose;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = TiDispatchInternal;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TiDispatch;

    DriverObject->DriverUnload = (PDRIVER_UNLOAD)TiUnload;

    return STATUS_SUCCESS;
}


VOID
#ifndef _MSC_VER
STDCALL
#endif
IPAddInterface(
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4)
{
    UNIMPLEMENTED
}


VOID
#ifndef _MSC_VER
STDCALL
#endif
IPDelInterface(
	DWORD	Unknown0)
{
    UNIMPLEMENTED
}


VOID
#ifndef _MSC_VER
STDCALL
#endif
LookupRoute(
	DWORD	Unknown0,
	DWORD	Unknown1)
{
    UNIMPLEMENTED
}

/* EOF */
