#include "driver.h"

NTSTATUS
SklHdAudBusEvtDeviceAdd(
	_In_ WDFDRIVER Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit
)
{
	UNREFERENCED_PARAMETER(Driver);
	NTSTATUS status;

	//
	// Initialize all the properties specific to the device.
	// Framework has default values for the one that are not
	// set explicitly here. So please read the doc and make sure
	// you are okay with the defaults.
	//
	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);

	status = Fdo_Create(DeviceInit);
	return status;
}

extern "C" NTSTATUS
DriverEntry(
__in PDRIVER_OBJECT  DriverObject,
__in PUNICODE_STRING RegistryPath
)
{
	WDF_DRIVER_CONFIG config;
	NTSTATUS status;
	WDFDRIVER wdfDriver;

	SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
		"Driver Entry\n");

	//
	//  Default to NonPagedPoolNx for non paged pool allocations where supported.
	//

	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	WDF_DRIVER_CONFIG_INIT(&config, SklHdAudBusEvtDeviceAdd);

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		WDF_NO_OBJECT_ATTRIBUTES,
		&config,
		&wdfDriver
	);
	if (!NT_SUCCESS(status))
	{
		SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
			"WdfDriverCreate failed %x\n", status);
	}

	return status;
}
