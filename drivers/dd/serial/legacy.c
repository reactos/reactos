/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/bus/serial/legacy.c
 * PURPOSE:         Legacy serial port enumeration
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 *                  Mark Junker (mjscod@gmx.de)
 */

#define NDEBUG
#include "serial.h"

UART_TYPE
SerialDetectUartType(
	IN PUCHAR BaseAddress)
{
	UCHAR Lcr, TestLcr;
	UCHAR OldScr, Scr5A, ScrA5;
	BOOLEAN FifoEnabled;
	UCHAR NewFifoStatus;
	
	Lcr = READ_PORT_UCHAR(SER_LCR(BaseAddress));
	WRITE_PORT_UCHAR(SER_LCR(BaseAddress), Lcr ^ 0xFF);
	TestLcr = READ_PORT_UCHAR(SER_LCR(BaseAddress)) ^ 0xFF;
	WRITE_PORT_UCHAR(SER_LCR(BaseAddress), Lcr);
	
	/* Accessing the LCR must work for a usable serial port */
	if (TestLcr != Lcr)
		return UartUnknown;
	
	/* Ensure that all following accesses are done as required */
	READ_PORT_UCHAR(SER_RBR(BaseAddress));
	READ_PORT_UCHAR(SER_IER(BaseAddress));
	READ_PORT_UCHAR(SER_IIR(BaseAddress));
	READ_PORT_UCHAR(SER_LCR(BaseAddress));
	READ_PORT_UCHAR(SER_MCR(BaseAddress));
	READ_PORT_UCHAR(SER_LSR(BaseAddress));
	READ_PORT_UCHAR(SER_MSR(BaseAddress));
	READ_PORT_UCHAR(SER_SCR(BaseAddress));
	
	/* Test scratch pad */
	OldScr = READ_PORT_UCHAR(SER_SCR(BaseAddress));
	WRITE_PORT_UCHAR(SER_SCR(BaseAddress), 0x5A);
	Scr5A = READ_PORT_UCHAR(SER_SCR(BaseAddress));
	WRITE_PORT_UCHAR(SER_SCR(BaseAddress), 0xA5);
	ScrA5 = READ_PORT_UCHAR(SER_SCR(BaseAddress));
	WRITE_PORT_UCHAR(SER_SCR(BaseAddress), OldScr);
	
	/* When non-functional, we have a 8250 */
	if (Scr5A != 0x5A || ScrA5 != 0xA5)
		return Uart8250;
	
	/* Test FIFO type */
	FifoEnabled = (READ_PORT_UCHAR(SER_IIR(BaseAddress)) & 0x80) != 0;
	WRITE_PORT_UCHAR(SER_FCR(BaseAddress), SR_FCR_ENABLE_FIFO);
	NewFifoStatus = READ_PORT_UCHAR(SER_IIR(BaseAddress)) & 0xC0;
	if (!FifoEnabled)
		WRITE_PORT_UCHAR(SER_FCR(BaseAddress), 0);
	switch (NewFifoStatus)
	{
		case 0x00:
			return Uart16450;
		case 0x80:
			return Uart16550;
	}
	
	/* FIFO is only functional for 16550A+ */
	return Uart16550A;
}

NTSTATUS
DetectLegacyDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN ULONG ComPortBase,
	IN ULONG Irq)
{
	ULONG ResourceListSize;
	PCM_RESOURCE_LIST ResourceList;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
	BOOLEAN ConflictDetected;
	UART_TYPE UartType;
	PDEVICE_OBJECT Pdo = NULL;
	PDEVICE_OBJECT Fdo;
	KIRQL Dirql;
	NTSTATUS Status;
	
	/* Create resource list */
	ResourceListSize = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
	ResourceList = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(PagedPool, ResourceListSize, SERIAL_TAG);
	if (!ResourceList)
		return STATUS_INSUFFICIENT_RESOURCES;
	ResourceList->Count = 1;
	ResourceList->List[0].InterfaceType = Isa;
	ResourceList->List[0].BusNumber = -1; /* FIXME */
	ResourceList->List[0].PartialResourceList.Version = 1;
	ResourceList->List[0].PartialResourceList.Revision = 1;
	ResourceList->List[0].PartialResourceList.Count = 2;
	ResourceDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[0];
	ResourceDescriptor->Type = CmResourceTypePort;
	ResourceDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
	ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
	ResourceDescriptor->u.Port.Start.u.HighPart = 0;
	ResourceDescriptor->u.Port.Start.u.LowPart = ComPortBase;
	ResourceDescriptor->u.Port.Length = 8;
	
	ResourceDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[1];
	ResourceDescriptor->Type = CmResourceTypeInterrupt;
	ResourceDescriptor->ShareDisposition = CmResourceShareShared;
	ResourceDescriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
	ResourceDescriptor->u.Interrupt.Vector = HalGetInterruptVector(
		Internal, 0, 0, Irq,
		&Dirql,
		&ResourceDescriptor->u.Interrupt.Affinity);
	ResourceDescriptor->u.Interrupt.Level = (ULONG)Dirql;
	
	/* Report resource list */
	Status = IoReportResourceForDetection(
		DriverObject, ResourceList, ResourceListSize,
		NULL, NULL, 0,
		&ConflictDetected);
	if (Status == STATUS_CONFLICTING_ADDRESSES)
		return STATUS_DEVICE_NOT_CONNECTED;
	if (!NT_SUCCESS(Status))
		return Status;
	
	/* Test if port exists */
	UartType = SerialDetectUartType((PUCHAR)ComPortBase);
	
	/* Report device if detected... */
	if (UartType != UartUnknown)
	{
		Status = IoReportDetectedDevice(
			DriverObject,
			ResourceList->List[0].InterfaceType, ResourceList->List[0].BusNumber, -1/*FIXME*/,
			ResourceList, NULL,
			TRUE,
			&Pdo);
		if (NT_SUCCESS(Status))
		{
			Status = SerialAddDeviceInternal(DriverObject, Pdo, UartType, &Fdo);
			if (NT_SUCCESS(Status))
			{
				Status = SerialPnpStartDevice(Fdo, ResourceList);
			}
		}
	}
	else
	{
		/* Release resources */
		Status = IoReportResourceForDetection(
			DriverObject, NULL, 0,
			NULL, NULL, 0,
			&ConflictDetected);
		Status = STATUS_DEVICE_NOT_CONNECTED;
	}
	return Status;
}

NTSTATUS
DetectLegacyDevices(
	IN PDRIVER_OBJECT DriverObject)
{
	ULONG ComPortBase[] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
	ULONG Irq[] = { 4, 3, 4, 3 };
	ULONG i;
	NTSTATUS Status;
	NTSTATUS ReturnedStatus = STATUS_SUCCESS;
	
	for (i = 0; i < sizeof(ComPortBase)/sizeof(ComPortBase[0]); i++)
	{
		Status = DetectLegacyDevice(DriverObject, ComPortBase[i], Irq[i]);
		if (!NT_SUCCESS(Status) && Status != STATUS_DEVICE_NOT_CONNECTED)
			ReturnedStatus = Status;
		DPRINT("Serial: Legacy device at 0x%x (IRQ %lu): status = 0x%08lx\n", ComPortBase[i], Irq[i], Status);
	}
	
	return ReturnedStatus;
}
