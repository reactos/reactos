/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/rawchan.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
RawChannelCreate(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RawChannelDestroy(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RawChannelORead(
	IN PSAC_CHANNEL Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize,
	OUT PULONG ByteCount
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RawChannelOEcho(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR String,
	IN ULONG Length
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RawChannelOWrite2(
	IN PSAC_CHANNEL Channel,
	IN PCHAR String,
	IN ULONG Size
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RawChannelOFlush(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

ULONG
RawChannelGetIBufferIndex(
	IN PSAC_CHANNEL Channel
	)
{
	return 0;
}

VOID
RawChannelSetIBufferIndex(
	IN PSAC_CHANNEL Channel,
	IN ULONG BufferIndex
	)
{

}

NTSTATUS
RawChannelOWrite(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR String,
	IN ULONG Length
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RawChannelIRead(
	IN PSAC_CHANNEL Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize,
	IN PULONG ReturnBufferSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RawChannelIBufferIsFull(
	IN PSAC_CHANNEL Channel,
	PBOOLEAN BufferStatus
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

ULONG
RawChannelIBufferLength(
	IN PSAC_CHANNEL Channel
	)
{
	return 0;
}

CHAR
RawChannelIReadLast(
	IN PSAC_CHANNEL Channel
	)
{
	return 0;
}

NTSTATUS
RawChannelIWrite(
	IN PSAC_CHANNEL Channel,
	IN PCHAR Buffer,
	IN ULONG BufferSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}
