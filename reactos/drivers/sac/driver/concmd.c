/*
 * PROJECT:		 ReactOS Boot Loader
 * LICENSE:		 BSD - See COPYING.ARM in the top level directory
 * FILE:		 drivers/sac/driver/concmd.c
 * PURPOSE:		 Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS:	 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

NTSTATUS
GetTListInfo(
	IN PVOID TListData,
	IN ULONG InputSize,
	IN ULONG TotalSize
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
PrintTListInfo(
	IN PVOID TListData
	)
{

}

VOID
PutMore(
	OUT PBOOLEAN ScreenFull
	)
{

}

BOOLEAN
RetrieveIpAddressFromString(
	IN PWCHAR IpString,
	OUT PULONG IpAddress
	)
{
	return FALSE;
}

NTSTATUS
CallQueryIPIOCTL(
	IN HANDLE DriverHandle,
	IN PVOID DriverObject,
	IN HANDLE WaitEvent,
	IN PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID InputBuffer,
	IN ULONG InputBufferLength,
	IN PVOID OutputBuffer,
	IN ULONG OutputBufferLength, 
	IN BOOLEAN PrintMessage,
	OUT PBOOLEAN MessagePrinted
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
DoFullInfoCommand(
	VOID
	)
{

}

VOID
DoPagingCommand(
	VOID
	)
{

}

VOID
DoSetTimeCommand(
	IN PCHAR InputTime
	)
{

}

VOID
DoKillCommand(
	IN PCHAR KillString
	)
{

}

VOID
DoLowerPriorityCommand(
	IN PCHAR PrioString
	)
{

}

VOID
DoRaisePriorityCommand(
	IN PCHAR PrioString
	)
{

}

VOID
DoLimitMemoryCommand(
	IN PCHAR LimitString
	)
{

}

VOID
DoRebootCommand(
	IN BOOLEAN Reboot
	)
{

}

VOID
DoCrashCommand(
	VOID
	)
{

}

VOID
DoMachineInformationCommand(
	VOID
	)
{

}

NTSTATUS
DoChannelListCommand(
	VOID
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DoChannelCloseByNameCommand(
	IN PCHAR Count
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DoChannelCloseByIndexCommand(
	IN ULONG ChannelIndex
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DoChannelSwitchByNameCommand(
	IN PCHAR Count
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DoChannelSwitchByIndexCommand(
	IN ULONG ChannelIndex
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

VOID
DoChannelCommand(
	IN PCHAR ChannelString
	)
{

}

VOID
DoCmdCommand(
	VOID
	)
{

}

VOID
DoLockCommand(
	VOID
	)
{

}

VOID
DoHelpCommand(
	VOID
	)
{

}

VOID
DoGetNetInfo(
	IN BOOLEAN DoPrint
	)
{

}

VOID
DoSetIpAddressCommand(
	IN PCHAR IpString
	)
{
	
}

VOID
DoTlistCommand(
	VOID
	)
{
	
}