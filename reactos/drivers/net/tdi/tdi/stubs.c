
/* $Id: stubs.c,v 1.5 2002/09/07 15:12:07 chorns Exp $
 *
 */
#include <ddk/ntddk.h>
#include <ddk/tdi.h>

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
	IN	ULONG	SourceOffset,
	IN	ULONG	SourceBytesToCopy,
	IN	PMDL	DestinationMdlChain,
	IN	ULONG	DestinationOffset,
	IN	PULONG	BytesCopied
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS 
STDCALL
TdiCopyMdlToBuffer (
	IN	PMDL	SourceMdlChain,
	IN	ULONG	SourceOffset,
	IN	PVOID	DestinationBuffer,
	IN	ULONG	DestinationOffset,
	IN	ULONG	DestinationBufferSize,
	OUT	PULONG	BytesCopied
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


VOID
STDCALL
TdiInitialize (
	PVOID	Unknown0
	)
{
}


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


VOID
STDCALL
TdiOpenNetbiosAddress (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
}


VOID
STDCALL
TdiReturnChainedReceives (
	IN	PVOID	* TsduDescriptors,
	IN	ULONG	NumberOfTsdus
        )
{
}


/* EOF */
