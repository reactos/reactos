/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/init.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	)
{
	HEADLESS_RSP_QUERY_INFO HeadlessInformation;
	ULONG InfoSize;
	NTSTATUS Status;
	UNICODE_STRING DriverName;
	PDEVICE_OBJECT DeviceObject;
	PSAC_DEVICE_EXTENSION DeviceExtension;
	PAGED_CODE();

	SAC_DBG(SAC_DBG_ENTRY_EXIT, "Entering.\n");

	HeadlessDispatch(
		HeadlessCmdQueryInformation,
		NULL,
		0,
		&HeadlessInformation,
		&InfoSize
		);
	if ((HeadlessInformation.Serial.TerminalType != HeadlessUndefinedPortType) &&
		((HeadlessInformation.Serial.TerminalType != HeadlessSerialPort) ||
		 (HeadlessInformation.Serial.TerminalAttached)))
	{
		RtlInitUnicodeString(&DriverName, L"\\Device\\SAC");

		Status = IoCreateDevice(
			DriverObject,
			sizeof(SAC_DEVICE_EXTENSION),
			&DriverName,
			FILE_DEVICE_UNKNOWN,
			FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&DeviceObject
			);
		if (NT_SUCCESS(Status))
		{
			DeviceExtension = DeviceObject->DeviceExtension;
			DeviceExtension->Initialized = FALSE;

			RtlFillMemoryUlong(
				DriverObject->MajorFunction,
				sizeof(DriverObject->MajorFunction) / sizeof(PVOID),
				(ULONG_PTR)Dispatch);
			DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
				DispatchDeviceControl;
			DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] =
				DispatchShutdownControl;
			DriverObject->FastIoDispatch = NULL;
			DriverObject->DriverUnload = UnloadHandler;

			if (InitializeGlobalData(RegistryPath, DriverObject))
			{
				if (InitializeDeviceData(DeviceObject))
				{
					IoRegisterShutdownNotification(DeviceObject);
					return Status;
				}
			}

			Status = STATUS_INSUFFICIENT_RESOURCES;
		}
		else
		{
			SAC_DBG(SAC_DBG_INIT, "unable to create device object: %X\n", Status);
		}

		FreeGlobalData();
		SAC_DBG(SAC_DBG_ENTRY_EXIT, "Exiting with status 0x%x\n", Status);
		return Status;
	}

	return STATUS_PORT_DISCONNECTED;
}
