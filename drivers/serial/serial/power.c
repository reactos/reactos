/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial port driver
 * FILE:            drivers/dd/serial/power.c
 * PURPOSE:         Serial IRP_MJ_POWER operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "serial.h"

#include <debug.h>

NTSTATUS NTAPI
SerialPower(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PSERIAL_DEVICE_EXTENSION DeviceExtension;

	TRACE_(SERIAL, "IRP_MJ_POWER dispatch\n");

	DeviceExtension = DeviceObject->DeviceExtension;
	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(DeviceExtension->LowerDevice, Irp);
}
