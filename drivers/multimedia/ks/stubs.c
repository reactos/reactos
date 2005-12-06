#include "ks.h"


typedef struct _KSDEVICE_DESCRIPTOR
{
} KSDEVICE_DESCRIPTOR, *PKSDEVICE_DESCRIPTOR;

typedef struct _KSDEVICE
{
} KSDEVICE, *PKSDEVICE;


NTSTATUS
KsAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	/* KsCreateDevice(DriverObject, PhysicalDeviceObject, ...); */

	return STATUS_NOT_IMPLEMENTED;
}



NTSTATUS
KsInitializeDriver(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath,
	IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
KsInitializeDevice(
	IN PDEVICE_OBJECT FunctionalDeviceObject,
	IN PDEVICE_OBJECT PhysicalDeviceObject,
	IN PDEVICE_OBJECT NextDeviceObject,
	IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL)
{
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
KsCreateDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT PhysicalDeviceObject,
	IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL,
	IN ULONG ExtensionSize OPTIONAL,
	OUT PKSDEVICE* Device OPTIONAL)
{
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
KsTerminateDevice(
	IN PDEVICE_OBJECT DeviceObject)
{
	return STATUS_NOT_IMPLEMENTED;
}



VOID
KsAcquireDevice(
	IN PKSDEVICE Device)
{
	/* Acquire device mutex */
}

VOID
KsReleaseDevice(
	IN PKSDEVICE Device)
{
	/* Releases device mutex and exits critical region */
}

VOID
KsAcquireControl(
	IN PVOID Object)
{
	/* Acquire filter control mutex for Object */
	/* Object should be pointed to a KSFILTER or KSPIN */
}


PKSDEVICE
KsGetDevice(
	IN PVOID Object)
{
	/* ? */
	return 0;
}

// inline
/*
PKSDEVICE __inline
KsFilterGetDevice(
	IN PKSFILTER Filter)
{
	return KsGetDevice((PVOID) Filter);
}

PKSDEVICE __inline
KsPinGetDevice(
	IN PKSPIN Pin)
{
	return KsGetDevice((PVOID) Pin);
}
*/
