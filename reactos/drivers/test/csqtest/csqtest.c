/*
 * CSQ Test Driver
 * Copyright (c) 2004, Vizzini (vizzini@plasmic.com)
 * Released under the GNU GPL for the ReactOS project
 *
 * This driver is designed to exercise the cancel-safe IRP queue logic.
 * Please refer to reactos/include/ddk/csq.h and reactos/drivers/lib/csq.
 */
#include <ntddk.h>
#include <ddk/csq.h>

/* XXX shortcomings in our headers... */
#define assert(x)
#ifndef KdPrint
#define KdPrint(x) DbgPrint x
#endif

/* Device name */
#define NT_DEVICE_NAME L"\\Device\\csqtest"

/* DosDevices name */
#define DOS_DEVICE_NAME L"\\??\\csqtest"

/* Global CSQ struct that the CSQ functions init */
IO_CSQ Csq;

/* List and lock for the actual IRP queue */
LIST_ENTRY IrpQueue;
KSPIN_LOCK IrpQueueLock;

/* Device object */
PDEVICE_OBJECT DeviceObject;

/* 
 * CSQ Callbacks 
 */
VOID NTAPI CsqInsertIrp(PIO_CSQ Csq, PIRP Irp)
{
	KdPrint(("Inserting IRP 0x%x into CSQ\n", Irp));
	InsertTailList(&IrpQueue, &Irp->Tail.Overlay.ListEntry);
}

VOID NTAPI CsqRemoveIrp(PIO_CSQ Csq, PIRP Irp)
{
	KdPrint(("Removing IRP 0x%x from CSQ\n", Irp));
	RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}

PIRP NTAPI CsqPeekNextIrp(PIO_CSQ Csq, PIRP Irp, PVOID PeekContext)
{
	KdPrint(("Peeking for next IRP\n"));

	if(Irp)
		return CONTAINING_RECORD(&Irp->Tail.Overlay.ListEntry.Flink, IRP, Tail.Overlay.ListEntry);

	if(IsListEmpty(&IrpQueue))
		return NULL;

	return CONTAINING_RECORD(IrpQueue.Flink, IRP, Tail.Overlay.ListEntry);
}

VOID NTAPI CsqAcquireLock(PIO_CSQ Csq, PKIRQL Irql)
{
	KdPrint(("Acquiring spin lock\n"));
	KeAcquireSpinLock(&IrpQueueLock, Irql);
}

VOID NTAPI CsqReleaseLock(PIO_CSQ Csq, KIRQL Irql)
{
	KdPrint(("Releasing spin lock\n"));
	KeReleaseSpinLock(&IrpQueueLock, Irql);
}

VOID NTAPI CsqCompleteCancelledIrp(PIO_CSQ Csq, PIRP Irp)
{
	KdPrint(("cancelling irp 0x%x\n", Irp));
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS NTAPI CsqInsertIrpEx(PIO_CSQ Csq, PIRP Irp, PVOID InsertContext)
/*
 * FUNCTION: Insert into IRP queue, with extra context
 *
 * NOTE: Switch call in DriverEntry to IoCsqInitializeEx to use this
 */
{
	CsqInsertIrp(Csq, Irp);
	return STATUS_PENDING;
}

/*
 * DISPATCH ROUTINES
 */

NTSTATUS NTAPI DispatchCreateCloseCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION StackLocation = IoGetCurrentIrpStackLocation(Irp);

	if(StackLocation->MajorFunction == IRP_MJ_CLEANUP)
		{
			/* flush the irp queue */
			PIRP CurrentIrp;

			KdPrint(("csqtest: Cleanup received; flushing the IRP queue with cancel\n"));

			while((CurrentIrp = IoCsqRemoveNextIrp(&Csq, 0)))
				{
					CurrentIrp->IoStatus.Status = STATUS_CANCELLED;
					CurrentIrp->IoStatus.Information = 0;

					IoCompleteRequest(CurrentIrp, IO_NO_INCREMENT);
				}
		}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI DispatchReadWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	/* According to the cancel sample in the DDK, IoCsqInsertIrp() marks the irp pending */
	/* However, I think it's wrong. */
	IoMarkIrpPending(Irp);
	IoCsqInsertIrp(&Csq, Irp, 0);

	return STATUS_PENDING;
}

NTSTATUS NTAPI DispatchIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/* 
 * all IOCTL requests flush the irp queue 
 */ 
{
	PIRP CurrentIrp;

	KdPrint(("csqtest: Ioctl received; flushing the IRP queue with success\n"));

	while((CurrentIrp = IoCsqRemoveNextIrp(&Csq, 0)))
		{
			CurrentIrp->IoStatus.Status = STATUS_SUCCESS;
			CurrentIrp->IoStatus.Information = 0;

			IoCompleteRequest(CurrentIrp, IO_NO_INCREMENT);
		}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

VOID NTAPI Unload(PDRIVER_OBJECT DriverObject)
/*
 * Function: called by the OS to release resources before unload
 */
{
	UNICODE_STRING LinkName;

	RtlInitUnicodeString(&LinkName, DOS_DEVICE_NAME);

	IoDeleteSymbolicLink(&LinkName);

	if(DeviceObject)
		IoDeleteDevice(DeviceObject);
}

/*
 * DriverEntry
 */

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status;
	UNICODE_STRING NtName;
	UNICODE_STRING DosName;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)DispatchCreateCloseCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)DispatchCreateCloseCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = (PDRIVER_DISPATCH)DispatchCreateCloseCleanup;
	DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH)DispatchReadWrite;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH)DispatchReadWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)DispatchIoctl;
	DriverObject->DriverUnload = (PDRIVER_UNLOAD)Unload;

	Status = IoCsqInitialize(&Csq, (PIO_CSQ_INSERT_IRP)CsqInsertIrp, CsqRemoveIrp, CsqPeekNextIrp, 
													 CsqAcquireLock, (PIO_CSQ_RELEASE_LOCK)CsqReleaseLock, CsqCompleteCancelledIrp);

	if(Status != STATUS_SUCCESS)
		KdPrint(("csqtest: IoCsqInitalize failed: 0x%x\n", Status));
	else
		KdPrint(("csqtest: IoCsqInitalize succeeded\n"));

	InitializeListHead(&IrpQueue);
	KeInitializeSpinLock(&IrpQueueLock);

	/* Set up a device */
	RtlInitUnicodeString(&NtName, NT_DEVICE_NAME);
	Status = IoCreateDevice(DriverObject, 0, &NtName, FILE_DEVICE_UNKNOWN, 0, 0, &DeviceObject);

	if(!NT_SUCCESS(Status))
		{
			KdPrint(("csqtest: Unable to create device: 0x%x\n", Status));
			return Status;
		}

	RtlInitUnicodeString(&DosName, DOS_DEVICE_NAME);
	Status = IoCreateSymbolicLink(&DosName, &NtName);

	if(!NT_SUCCESS(Status))
		{
			KdPrint(("csqtest: Unable to create link: 0x%x\n", Status));
			return Status;
		}
	
	DeviceObject->Flags |= DO_BUFFERED_IO;

	return STATUS_SUCCESS;
}

