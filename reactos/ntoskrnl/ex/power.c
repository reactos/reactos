/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/power.c
 * PURPOSE:         Power managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Added reboot support 30/01/99
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/io.h>
#include <internal/mm.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL 
NtSetSystemPowerState(VOID)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL 
NtShutdownSystem(IN SHUTDOWN_ACTION Action)
{
   if (Action > ShutdownPowerOff)
     return STATUS_INVALID_PARAMETER;

   IoShutdownRegisteredDevices();
   CmShutdownRegistry();
   IoShutdownRegisteredFileSystems();
   PiShutdownProcessManager();
   MiShutdownMemoryManager();
   
   if (Action == ShutdownNoReboot)
     {
#if 0
        /* Switch off */
        HalReturnToFirmware (FIRMWARE_OFF);
#endif
     }
   else if (Action == ShutdownReboot)
     {
        HalReturnToFirmware (FIRMWARE_REBOOT);
     }
   else
     {
        HalReturnToFirmware (FIRMWARE_HALT);
     }
   
   return STATUS_SUCCESS;
}

/* EOF */

