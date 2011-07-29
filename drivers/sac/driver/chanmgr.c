/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/chanmgr.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
ChanMgrInitialize(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrGetChannelIndex(
	IN PSAC_CHANNEL Channel,
	IN PULONG ChannelIndex
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrReleaseChannel(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrGetByHandle(
	IN PSAC_CHANNEL_ID ChannelId,
	OUT PSAC_CHANNEL pChannel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrGetByHandleAndFileObject(
	IN PSAC_CHANNEL_ID ChannelId,
	IN PFILE_OBJECT FileObject,
	OUT PSAC_CHANNEL pChannel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrGetChannelByName(
	IN PWCHAR Name,
	OUT PSAC_CHANNEL pChannel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrGetByIndex(
	IN ULONG TargetIndex,
	IN PSAC_CHANNEL *TargetChannel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrGetNextActiveChannel(
	IN PSAC_CHANNEL CurrentChannel,
	IN PULONG TargetIndex,
	OUT PSAC_CHANNEL *TargetChannel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
ChanMgrChannelDestroy(
	IN PSAC_CHANNEL Channel
	)
{

}

NTSTATUS
ChanMgrCloseChannel(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrReapChannel(
	IN ULONG ChannelIndex
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrReapChannels(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrGetChannelCount(
	OUT PULONG ChannelCount
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrIsFull(
	OUT PBOOLEAN IsFull
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrShutdown(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
ChanMgrIsUniqueName(
	IN PWCHAR ChannelName
	)
{
	return FALSE;
}

NTSTATUS
ChanMgrGenerateUniqueCmdName(
	IN PWCHAR ChannelName
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrCreateChannel(
	OUT PSAC_CHANNEL *Channel,
	IN PSAC_CHANNEL_ATTRIBUTES Attributes
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ChanMgrCloseChannelsWithFileObject(
	IN PFILE_OBJECT FileObject
	)
{
	return STATUS_NOT_IMPLEMENTED;
}
