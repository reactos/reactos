/*
	ReactOS Kernel Streaming component

	Written (or rather, stubbed!) by Andrew Greenwood
	September 2005
*/

#include <ks.h>

KSDDKAPI NTSTATUS NTAPI
KsCreatePin(
	IN HANDLE FilterHandle,
	IN PKSPIN_CONNECT Connect,
	IN ACCESS_MASK DesiredAccess,
	OUT PHANDLE ConnectionHandle)
{
}

KSDDKAPI NTSTATUS NTAPI
KsHandleSizedListQuery(
	IN PIRP Irp,
	IN ULONG DataItemsCount,
	IN ULONG DataItemSize,
	IN const VOID* DataItems)
{
}

KSDDKAPI NTSTATUS NTAPI
KsPinDataIntersection(
	IN PIRP Irp,
	IN PKSP_PIN Pin,
	OUT PVOID Data,
	IN ULONG DescriptorsCount,
	IN const KSPIN_DESCRIPTOR* Descriptor,
	IN PFNKSINTERSECTHANDLER IntersectHandler)
{
}

KSDDKAPI NTSTATUS NTAPI
KsPinPropertyHandler(
	IN PIRP Irp,
	IN PKSPROPERTY Property,
	IN OUT PVOID Data,
	IN ULONG DescriptorsCount,
	IN const KSPIN_DESCRIPTOR* Descriptor)
{
}

KSDDKAPI NTSTATUS NTAPI
KsValidateConnectRequest(
	IN PIRP Irp,
	IN ULONG DescriptorsCount,
	IN KSPIN_DESCRIPTOR* Descriptor,
	OUT PKSPIN_CONNECT* Connect)
{
}

