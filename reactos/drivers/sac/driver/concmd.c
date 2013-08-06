/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/concmd.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "sacdrv.h"

/* GLOBALS *******************************************************************/

PWCHAR GlobalBuffer;
ULONG GlobalBufferSize;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
DoRebootCommand(IN BOOLEAN Reboot)
{
    LARGE_INTEGER Timeout, TickCount;
    NTSTATUS Status;
    KEVENT Event;
    SAC_DBG(1, "SAC DoRebootCommand: Entering.\n");

    /* Get the current time now, and setup a timeout in 1 second */
    KeQueryTickCount(&TickCount);
    Timeout.QuadPart = TickCount.QuadPart / (10000000 / KeQueryTimeIncrement());

    /* Check if the timeout is small enough */
    if (Timeout.QuadPart < 60 )
    {
        /* Show the prompt */
        ConMgrSimpleEventMessage(Reboot ?
                                 SAC_RESTART_PROMPT : SAC_SHUTDOWN_PROMPT,
                                 TRUE);

        /* Do the wait */
        KeInitializeEvent(&Event, SynchronizationEvent, 0);
        Timeout.QuadPart = -10000000 * (60 - Timeout.LowPart);
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &Timeout);
    }

    /* Do a shutdown or a reboot, based on the request */
    Status = NtShutdownSystem(Reboot ? ShutdownReboot : ShutdownPowerOff);

    /* Check if anyone in the command channel already allocated this */
    if (!GlobalBuffer)
    {
        /* Allocate it */
        GlobalBuffer = SacAllocatePool(PAGE_SIZE, GLOBAL_BLOCK_TAG);
        if (!GlobalBuffer)
        {
            /* We need the global buffer, bail out without it*/
            SacPutSimpleMessage(SAC_OUT_OF_MEMORY_PROMPT);
            SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoRebootCommand: Exiting (1).\n");
            return;
        }

        /* Set the size of the buffer */
        GlobalBufferSize = PAGE_SIZE;
    }

    /* We came back from a reboot, this doesn't make sense, tell the user */
    SacPutSimpleMessage(Reboot ? SAC_RESTART_FAIL_PROMPT : SAC_SHUTDOWN_FAIL_PROMPT);
    swprintf(GlobalBuffer, GetMessage(SAC_FAIL_PROMPT), Status);
    SacPutString(GlobalBuffer);
    SAC_DBG(SAC_DBG_ENTRY_EXIT, "SAC DoRebootCommand: Exiting.\n");
}

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