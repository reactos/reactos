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
#include <roscfg.h>
#include <internal/ps.h>
#include <internal/io.h>
#include <internal/mm.h>
#include <internal/po.h>
#include <internal/cc.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL 
NtSetSystemPowerState(IN POWER_ACTION SystemAction,
		      IN SYSTEM_POWER_STATE MinSystemState,
		      IN ULONG Flags)
{
  /* Windows 2000 only */
  return(STATUS_NOT_IMPLEMENTED);
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
#else
        PopSetSystemPowerState(PowerSystemShutdown);
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

