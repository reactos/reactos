/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/conmgr.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
ConMgrShutdown(
	VOID)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrDisplayFastChannelSwitchingInterface(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrSetCurrentChannel(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrDisplayCurrentChannel(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrAdvanceCurrentChannel(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
ConMgrIsWriteEnabled(
	IN PSAC_CHANNEL Channel
	)
{
	return FALSE;
}

VOID
SacPutString(
	IN PWCHAR String
	)
{

}

BOOLEAN
SacPutSimpleMessage(
	IN ULONG MessageIndex
	)
{
	return FALSE;
}

NTSTATUS
ConMgrChannelOWrite(
	IN PSAC_CHANNEL Channel,
	IN PVOID WriteBuffer
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrGetChannelCloseMessage(
	IN PSAC_CHANNEL Channel,
	IN NTSTATUS CloseStatus,
	OUT PWCHAR OutputBuffer
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrWriteData(
	IN PSAC_CHANNEL Channel,
	IN PVOID Buffer,
	IN ULONG BufferLength
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrFlushData(
	IN PSAC_CHANNEL Channel
	)
{
	return FALSE;
}

BOOLEAN
ConMgrIsSacChannel(
	IN PSAC_CHANNEL Channel
	)
{
	return FALSE;
}

NTSTATUS
ConMgrInitialize(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrResetCurrentChannel(
	IN BOOLEAN KeepChannel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
ConMgrProcessInputLine(
	VOID
	)
{

}

VOID
ConMgrEventMessage(
	IN PWCHAR EventMessage,
	IN BOOLEAN LockHeld
	)
{

}

BOOLEAN
ConMgrSimpleEventMessage(
	IN ULONG MessageIndex,
	IN BOOLEAN LockHeld
	)
{
	return FALSE;
}

NTSTATUS
ConMgrChannelClose(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ConMgrHandleEvent(
	IN ULONG EventCode,
	IN PSAC_CHANNEL Channel,
	OUT PVOID Data
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
ConMgrWorkerProcessEvents(
	IN PSAC_CHANNEL Channel
	)
{

}
