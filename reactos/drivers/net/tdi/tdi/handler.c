/* $Id: handler.c,v 1.5 2003/07/10 19:54:13 chorns Exp $
 *
 * DESCRIPTION: Default TDI event handlers.
 */
#include <ntos.h>
#include <net/tdi.h>


/*
 * ClientEventChainedReceiveDatagram
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
TdiDefaultChainedRcvDatagramHandler (
	IN	PVOID	TdiEventContext,
	IN	LONG	SourceAddressLength,
	IN	PVOID	SourceAddress,
	IN	LONG	OptionsLength,
	IN	PVOID	Options,
	IN	ULONG	ReceiveDatagramFlags,
	IN	ULONG	ReceiveDatagramLength,
	IN	ULONG	StartingOffset,
	IN	PMDL	Tsdu,
	IN	PVOID	TsduDescriptor
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventChainedReceiveExpedited
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
TdiDefaultChainedRcvExpeditedHandler (
	IN	PVOID			TdiEventContext,
	IN	CONNECTION_CONTEXT	ConnectionContext,
	IN	ULONG			ReceiveFlags,
	IN	ULONG			ReceiveLength,
	IN	ULONG			StartingOffset,
	IN	PMDL			Tsdu,
	IN	PVOID			TsduDescriptor
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventChainedReceive
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
TdiDefaultChainedReceiveHandler (
	IN	PVOID			TdiEventContext,
	IN	CONNECTION_CONTEXT	ConnectionContext,
	IN	ULONG			ReceiveFlags,
	IN	ULONG			ReceiveLength,
	IN	ULONG			StartingOffset,
	IN	PMDL			Tsdu,
	IN	PVOID			TsduDescriptor
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventConnect
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
TdiDefaultConnectHandler (
	IN	PVOID			TdiEventContext,
        IN	LONG			RemoteAddressLength,
        IN	PVOID			RemoteAddress,
        IN	LONG			UserDataLength,
        IN	PVOID			UserData,
	IN	LONG			OptionsLength,
	IN	PVOID			Options,
	OUT	CONNECTION_CONTEXT	* ConnectionContext,
	OUT	PIRP			* AcceptIrp
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventDisconnect
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
TdiDefaultDisconnectHandler (
	IN	PVOID			TdiEventContext,
	IN	CONNECTION_CONTEXT	ConnectionContext,
	IN	LONG			DisconnectDataLength,
	IN	PVOID			DisconnectData,
	IN	LONG			DisconnectInformationLength,
	IN	PVOID			DisconnectInformation,
	IN	ULONG			DisconnectFlags
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventError
 *
 * @unimplemented
 */
NTSTATUS 
STDCALL
TdiDefaultErrorHandler (
	IN	PVOID		TdiEventContext,
	IN	NTSTATUS	Status
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventReceiveDatagram
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
TdiDefaultRcvDatagramHandler (
	IN	PVOID	TdiEventContext,
	IN	LONG	SourceAddressLength,
	IN	PVOID	SourceAddress,
	IN	LONG	OptionsLength,
	IN	PVOID	Options,
	IN	ULONG	ReceiveDatagramFlags,
	IN	ULONG	BytesIndicated,
	IN	ULONG	BytesAvailable,
	OUT	ULONG	* BytesTaken,
	IN	PVOID	Tsdu,
	OUT	PIRP	* IoRequestPacket
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventReceiveExpedited
 *
 * @unimplemented
 */
TDI_STATUS
STDCALL
TdiDefaultRcvExpeditedHandler (
	IN	PVOID			TdiEventContext,
	IN	CONNECTION_CONTEXT	ConnectionContext,
	IN	ULONG			ReceiveFlags,
	IN	ULONG			BytesIndicated,
	IN	ULONG			BytesAvailable,
	OUT	ULONG			* BytesTaken,
	IN	PVOID			Tsdu,
	OUT	PIRP			* IoRequestPacket
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventReceive
 *
 * @unimplemented
 */
NTSTATUS 
STDCALL
TdiDefaultReceiveHandler (
	IN	PVOID			TdiEventContext,
	IN	CONNECTION_CONTEXT	ConnectionContext,
	IN	ULONG			ReceiveFlags,
	IN	ULONG			BytesIndicated,
	IN	ULONG			BytesAvailable,
	OUT	ULONG			* BytesTaken,
	IN	PVOID			Tsdu,
	OUT	PIRP			* IoRequestPacket
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * ClientEventSendPossible
 *
 * @unimplemented
 */
TDI_STATUS 
STDCALL
TdiDefaultSendPossibleHandler (
	IN	PVOID	TdiEventContext,
	IN	PVOID	ConnectionContext,
	IN	ULONG	BytesAvailable
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
