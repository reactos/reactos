
/* $Id$
 *
 */
#include <ntddk.h>
#include <tdi.h>

/*
 * @unimplemented
 */
VOID
NTAPI
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
NTAPI
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
NTAPI
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
NTAPI
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
NTAPI
TdiInitialize (
	PVOID	Unknown0
	)
{
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
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
NTAPI
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
NTAPI
TdiReturnChainedReceives (
	IN	PVOID	* TsduDescriptors,
	IN	ULONG	NumberOfTsdus
        )
{
}

/* EOF */
