/* $Id: stubs.c,v 1.1 1999/11/20 20:41:29 ea Exp $
 *
 */
#include <ntos.h>

VOID 
STDCALL
TdiBuildNetbiosAddress (
	IN	PUCHAR			NetbiosName,
	IN	BOOLEAN			IsGroupName,
	IN OUT	PTA_NETBIOS_ADDRESS	NetworkName
        )
{
}


NTSTATUS
STDCALL
TdiBuildNetbiosAddressEa (
	IN	PUCHAR	Buffer,
	IN	BOOLEAN	GroupName,
	IN	PUCHAR	NetbiosName
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS 
STDCALL
TdiCopyBufferToMdl (
	IN	PVOID	SourceBuffer,
        IN	ULONG  SourceOffset,
        IN	ULONG  SourceBytesToCopy,
        IN	PMDL  DestinationMdlChain,
        IN	ULONG  DestinationOffset,
        IN	PULONG  BytesCopied
        )
{
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS 
STDCALL
TdiCopyMdlToBuffer (
        IN PMDL  SourceMdlChain,
        IN ULONG  SourceOffset,
        IN PVOID  DestinationBuffer,
        IN ULONG  DestinationOffset,
        IN ULONG  DestinationBufferSize,
        OUT PULONG  BytesCopied
        )
{
	return STATUS_NOT_IMPLEMENTED;
}



/*
TdiDeregisterAddressChangeHandler
TdiDeregisterDeviceObject
TdiDeregisterNetAddress
TdiDeregisterNotificationHandler

TdiInitialize
*/

NTSTATUS
STDCALL
TdiMapUserRequest (
	IN	PDEVICE_OBJECT		DeviceObject,
	IN	PIRP			Irp,
	IN	PIO_STACK_LOCATION	IrpSp
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
TdiOpenNetbiosAddress
TdiRegisterAddressChangeHandler
TdiRegisterDeviceObject
TdiRegisterNetAddress
TdiRegisterNotificationHandler
*/

VOID
STDCALL
TdiReturnChainedReceives (
	IN	PVOID	* TsduDescriptors,
	IN	ULONG	NumberOfTsdus
        )
{
}


/* EOF */
