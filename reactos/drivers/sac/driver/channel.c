/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/channel.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

BOOLEAN
ChannelIsValidType(
	IN SAC_CHANNEL_TYPE ChannelType
	)
{
	return ((ChannelType >= VtUtf8) && (ChannelType <= Raw));
}

BOOLEAN
ChannelIsEqual(
	IN PSAC_CHANNEL Channel,
	IN PSAC_CHANNEL_ID ChannelId
	)
{
	return IsEqualGUIDAligned(
		&Channel->ChannelId.ChannelGuid,
		&ChannelId->ChannelGuid);
}

NTSTATUS
ChannelInitializeVTable(
	IN PSAC_CHANNEL Channel
	)
{
	 return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelDereferenceHandles(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelDestroy(
	IN PSAC_CHANNEL Channel
	)
{
	CHECK_PARAMETER(Channel);
	
	return ChannelDereferenceHandles(Channel);
}

NTSTATUS
ChannelOWrite(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR Buffer,
	IN ULONG BufferSize
	)
{
	NTSTATUS Status;
	
	CHECK_PARAMETER3(BufferSize < SAC_OBUFFER_SIZE);

	ChannelLockOBuffer(Channel);

	Status = Channel->OBufferWrite(Channel, Buffer, BufferSize);
	
	ChannelUnlockOBuffer(Channel);
	
	return Status;
}

NTSTATUS
ChannelOFlush(
	IN PSAC_CHANNEL Channel
	)
{
	NTSTATUS Status;
	
	ChannelLockOBuffer(Channel);

	Status = Channel->OBufferFlush(Channel);
	
	ChannelUnlockOBuffer(Channel);
	
	return Status;
}

NTSTATUS
ChannelIWrite(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR Buffer,
	IN ULONG BufferSize
	)
{
	NTSTATUS Status;

	ChannelLockIBuffer(Channel);

	Status = Channel->IBufferWrite(Channel, Buffer, BufferSize);
	
	ChannelUnlockIBuffer(Channel);
	
	return Status;
}

ULONG 
ChannelIRead(
	IN PSAC_CHANNEL Channel,
	PWCHAR Buffer,
	ULONG BufferSize,
	OUT PULONG ResultBufferSize
	)
{
	NTSTATUS Status;
	
	ChannelLockIBuffer(Channel);

	Status = Channel->IBufferRead(Channel, Buffer, BufferSize, ResultBufferSize);
	
	ChannelUnlockIBuffer(Channel);
	
	return Status;
}

NTSTATUS
ChannelIReadLast(
	IN PSAC_CHANNEL Channel
	)
{
	NTSTATUS Status;

	ChannelLockIBuffer(Channel);

	Status = Channel->IBufferReadLast(Channel);
	
	ChannelUnlockIBuffer(Channel);
	
	return Status;
}

ULONG
ChannelIBufferLength(
	IN PSAC_CHANNEL Channel
	)
{
	NTSTATUS Length;

	ChannelLockOBuffer(Channel);

	Length = Channel->IBufferLength(Channel);
	
	ChannelUnlockOBuffer(Channel);
	
	return Length;
}

NTSTATUS
ChannelGetName(
	IN PSAC_CHANNEL Channel,
	OUT PWCHAR *Name
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelSetName(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR Name
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelGetDescription(
	IN PSAC_CHANNEL Channel,
	OUT PWCHAR *Description
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelSetDescription(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR Description
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelSetStatus(
	IN PSAC_CHANNEL Channel,
	IN SAC_CHANNEL_STATUS ChannelStatus
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelGetStatus(
	IN PSAC_CHANNEL Channel,
	OUT PSAC_CHANNEL_STATUS ChannelStatus
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelGetApplicationType(
	IN PSAC_CHANNEL Channel,
	OUT PGUID ApplicationType
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
ChannelSetLockEvent(
	IN PSAC_CHANNEL Channel
	)
{
	NTSTATUS Status;
	
	ChannelSetEvent(Channel, LockEvent);
	
	return Status;
}

NTSTATUS
NTAPI
ChannelSetRedrawEvent(
	IN PSAC_CHANNEL Channel
	)
{
	NTSTATUS Status;

	ChannelSetEvent(Channel, RedrawEvent);

	return Status;
}

NTSTATUS
ChannelClearRedrawEvent(
	IN PSAC_CHANNEL Channel
	)
{
	NTSTATUS Status;
	
	ChannelClearEvent(Channel, RedrawEvent);
	
	return Status;
}

NTSTATUS
ChannelHasRedrawEvent(
	IN PSAC_CHANNEL Channel,
	OUT PBOOLEAN Present
	)
{
	CHECK_PARAMETER1(Channel);
	CHECK_PARAMETER2(Present);
	
	*Present = Channel->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT;
	
	return STATUS_SUCCESS;
}

BOOLEAN
ChannelIsActive(
	IN PSAC_CHANNEL Channel
	)
{
	SAC_CHANNEL_STATUS ChannelStatus;

	if (!NT_SUCCESS(ChannelGetStatus(Channel, &ChannelStatus))) return FALSE;

	return (ChannelStatus == Active);
}

BOOLEAN
ChannelIsClosed(
	IN PSAC_CHANNEL Channel
	)
{
	return FALSE;
}

NTSTATUS
ChannelCreate(
	IN PSAC_CHANNEL Channel,
	IN PSAC_CHANNEL_ATTRIBUTES Attributes,
	IN SAC_CHANNEL_ID ChannelId
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelClose(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}
