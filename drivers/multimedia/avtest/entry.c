#include <debug.h>
#include <ks.h>

/* Where do we go? */
#ifndef SIZEOF_ARRAY
	#define SIZEOF_ARRAY(array) \
		(sizeof(array) / sizeof(array[0]))
#endif

/* Not in the DDK but hey! */
#define DEFINE_KSFILTER_DISPATCH(name) \
	const KSFILTER_DISPATCH name =

/* To be put in KS.H */
#define DEFINE_KSFILTER_DESCRIPTOR(name) \
	const KSFILTER_DESCRIPTOR name =

#define DEFINE_KSFILTER_DESCRIPTOR_TABLE(name) \
	const KSFILTER_DESCRIPTOR* const name[] =



NTSTATUS FilterCreate(
	IN OUT PKSFILTER Filter,
	IN PIRP Irp)
{
	return STATUS_SUCCESS;
}

NTSTATUS FilterClose(
	IN OUT PKSFILTER Filter,
	IN PIRP Irp)
{
	return STATUS_SUCCESS;
}

NTSTATUS Process(
	IN PKSFILTER Filter,
	IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex)
{
	return STATUS_SUCCESS;
}


DEFINE_KSFILTER_DISPATCH(FilterDispatch)
{
	FilterCreate,
	FilterClose,
	Process,
	NULL	// Reset
};

DEFINE_KSFILTER_DESCRIPTOR(FilterDesc)
{
};

DEFINE_KSFILTER_DESCRIPTOR_TABLE(FilterDescs)
{
	&FilterDesc
};



const KSDEVICE_DESCRIPTOR DeviceDescriptor =
{
	NULL,
	SIZEOF_ARRAY(FilterDescs),
	FilterDescs
};


/* Funcs */

NTSTATUS STDCALL
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPathName)
{
	DPRINT1("AVStream test component loaded!\n");

	return KsInitializeDriver(DriverObject, RegistryPathName,
							&DeviceDescriptor);
}
