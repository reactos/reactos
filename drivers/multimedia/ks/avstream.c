#include <ks.h>
#include <debug.h>


NTSTATUS NTAPI
KsInitializeDriver(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath,
	IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL)
{
	DPRINT("KsInitializeDriver\n");

	/* This should set up IRPs etc. */

	return STATUS_NOT_IMPLEMENTED;
}
