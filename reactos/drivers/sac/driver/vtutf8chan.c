/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/vtutf8chan.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
VTUTF8ChannelCreate(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelDestroy(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelORead(
	IN PSAC_CHANNEL Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize,
	OUT PULONG ByteCount
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelOEcho(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR String,
	IN ULONG Size
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
VTUTF8ChannelScanForNumber(
	IN PWCHAR String,
	OUT PULONG Number
	)
{
	return FALSE;
}

NTSTATUS
VTUTF8ChannelAnsiDispatch(
	IN NTSTATUS Status,
	IN ULONG AnsiCode,
	IN PWCHAR Data,
	IN ULONG Length
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelProcessAttributes(
	IN PSAC_CHANNEL Channel,
	IN UCHAR Attribute
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

ULONG
VTUTF8ChannelGetIBufferIndex(
	IN PSAC_CHANNEL Channel
	)
{
	return 0;
}

VOID
VTUTF8ChannelSetIBufferIndex(
	IN PSAC_CHANNEL Channel,
	IN ULONG BufferIndex
	)
{

}

NTSTATUS
VTUTF8ChannelConsumeEscapeSequence(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR String
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelOFlush(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelIRead(
	IN PSAC_CHANNEL Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize,
	IN PULONG ReturnBufferSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelIBufferIsFull(
	IN PSAC_CHANNEL Channel,
	OUT PBOOLEAN BufferStatus
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

WCHAR
VTUTF8ChannelIReadLast(
	IN PSAC_CHANNEL Channel
	)
{
	return 0;
}

ULONG
VTUTF8ChannelIBufferLength(
	IN PSAC_CHANNEL Channel
	)
{
	return 0;
}

NTSTATUS
VTUTF8ChannelOWrite2(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR String,
	IN ULONG Size
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelIWrite(
	IN PSAC_CHANNEL Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
VTUTF8ChannelOWrite(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR String,
	IN ULONG Length
	)
{
	return STATUS_NOT_IMPLEMENTED;
}
