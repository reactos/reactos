
/* $Id$
 *
 */
#include <ntddk.h>
#include <tdi.h>

/*
 * @unimplemented
 */
VOID
STDCALL
TdiBuildNetbiosAddress (
	IN	PUCHAR			NetbiosName,
	IN	BOOLEAN			IsGroupName,
	IN OUT	PTA_NETBIOS_ADDRESS	NetworkName
        )
{
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
VOID
STDCALL
TdiInitialize (
	PVOID	Unknown0
	)
{
}


/*
 * @unimplemented
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
 * @unimplemented
 */
VOID
STDCALL
TdiOpenNetbiosAddress (
	ULONG   Unknown0,
	ULONG   Unknown1,
	ULONG   Unknown2,
	ULONG   Unknown3
	)
{
}


/*
 * @unimplemented
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
