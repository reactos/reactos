/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/bus/serial/legacy.c
 * PURPOSE:         Legacy serial port enumeration
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */

#define NDEBUG
#include "serial.h"

static BOOLEAN
SerialDoesPortExist(PUCHAR BaseAddress)
{
	BOOLEAN Found;
	BYTE Mcr;
	BYTE Msr;
	
	Found = FALSE;
	
	/* save Modem Control Register (MCR) */
	Mcr = READ_PORT_UCHAR(SER_MCR(BaseAddress));
	
	/* enable loop mode (set Bit 4 of the MCR) */
	WRITE_PORT_UCHAR(SER_MCR(BaseAddress), 0x10);
	
	/* clear all modem output bits */
	WRITE_PORT_UCHAR(SER_MCR(BaseAddress), 0x10);
	
	/* read the Modem Status Register */
	Msr = READ_PORT_UCHAR(SER_MSR(BaseAddress));
	
	/*
	 * the upper nibble of the MSR (modem output bits) must be
	 * equal to the lower nibble of the MCR (modem input bits)
	 */
	if ((Msr & 0xf0) == 0x00)
	{
		/* set all modem output bits */
		WRITE_PORT_UCHAR(SER_MCR(BaseAddress), 0x1f);
		
		/* read the Modem Status Register */
		Msr = READ_PORT_UCHAR(SER_MSR(BaseAddress));
		
		/*
		 * the upper nibble of the MSR (modem output bits) must be
		 * equal to the lower nibble of the MCR (modem input bits)
		 */
		if ((Msr & 0xf0) == 0xf0)
		{
			/*
			 * setup a resonable state for the port:
			 * enable fifo and clear recieve/transmit buffers
			 */
			WRITE_PORT_UCHAR(SER_FCR(BaseAddress),
				(SR_FCR_ENABLE_FIFO | SR_FCR_CLEAR_RCVR | SR_FCR_CLEAR_XMIT));
			WRITE_PORT_UCHAR(SER_FCR(BaseAddress), 0);
			READ_PORT_UCHAR(SER_RBR(BaseAddress));
			WRITE_PORT_UCHAR(SER_IER(BaseAddress), 0);
			Found = TRUE;
		}
	}
	
	/* restore MCR */
	WRITE_PORT_UCHAR(SER_MCR(BaseAddress), Mcr);
	
	return Found;
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
	BOOLEAN ConflictDetected, FoundPort;
	PDEVICE_OBJECT Pdo = NULL;
	PDEVICE_OBJECT Fdo;
	KIRQL Dirql;
	NTSTATUS Status;
	
	/* Create resource list */
	ResourceListSize = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
	ResourceList = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(NonPagedPool, ResourceListSize, SERIAL_TAG);
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
	if (ConflictDetected)
		return STATUS_DEVICE_NOT_CONNECTED;
	if (!NT_SUCCESS(Status))
		return Status;
	
	/* Test if port exists */
	FoundPort = SerialDoesPortExist((PUCHAR)ComPortBase);
	
	/* Report device if detected... */
	if (FoundPort)
	{
		Status = IoReportDetectedDevice(
			DriverObject,
			ResourceList->List[0].InterfaceType, ResourceList->List[0].BusNumber, -1/*FIXME*/,
			ResourceList, NULL,
			TRUE,
			&Pdo);
		if (NT_SUCCESS(Status))
		{
			Status = SerialAddDeviceInternal(DriverObject, Pdo, &Fdo);
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
