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

#include <ntoskrnl.h>
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

/*
 * @implemented
 */
NTSTATUS STDCALL 
NtShutdownSystem(IN SHUTDOWN_ACTION Action)
{
   static PCH FamousLastWords[] =
     {
       "Oh my God, they killed Kenny! Those bastards!\n",
       "Goodbye, and thanks for all the fish\n",
       "I'll be back\n"
     };
   LARGE_INTEGER Now;

   if (Action > ShutdownPowerOff)
     return STATUS_INVALID_PARAMETER;

   ZwQuerySystemTime(&Now);
   Now.u.LowPart = Now.u.LowPart >> 8; /* Seems to give a somewhat better "random" number */

   IoShutdownRegisteredDevices();
   CmShutdownRegistry();
   IoShutdownRegisteredFileSystems();

   PiShutdownProcessManager();
   MiShutdownMemoryManager();
   
   if (Action == ShutdownNoReboot)
     {
        HalReleaseDisplayOwnership();
        HalDisplayString(FamousLastWords[Now.u.LowPart % (sizeof(FamousLastWords) / sizeof(PCH))]);
#if 0
        /* Switch off */
        HalReturnToFirmware (FIRMWARE_OFF);
#else
        PopSetSystemPowerState(PowerSystemShutdown);

	while (TRUE)
	  {
	    Ke386DisableInterrupts();
	    Ke386HaltProcessor();
	  }
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

