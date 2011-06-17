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
	return FALSE;
}

BOOLEAN
ChannelIsEqual(
	IN PSAC_CHANNEL Channel,
	IN PSAC_CHANNEL_ID ChannelId
	)
{
	return FALSE;
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
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelOWrite(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR Buffer,
	IN ULONG BufferSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelOFlush(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelIWrite(
	IN PSAC_CHANNEL Channel,
	IN PWCHAR Buffer,
	IN ULONG BufferSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

ULONG 
ChannelIRead(
	IN PSAC_CHANNEL Channel,
	PWCHAR Buffer,
	ULONG BufferSize,
	OUT PULONG ResultBufferSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelIReadLast(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

ULONG
ChannelIBufferLength(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
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
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
ChannelSetRedrawEvent(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelClearRedrawEvent(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChannelHasRedrawEvent(
	IN PSAC_CHANNEL Channel,
	OUT PBOOLEAN Present
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
ChannelIsActive(
	IN PSAC_CHANNEL Channel
	)
{
	return FALSE;
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
