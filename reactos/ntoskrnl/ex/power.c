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
       "So long, and thanks for all the fish\n",
       "I think you ought to know I'm feeling very depressed\n",
       "I'm not getting you down at all am I?\n",
       "I'll be back\n",
       "It's the same series of signal over and over again!\n",
       "Pie Iesu Domine, dona eis requiem\n",
       "Wandering stars, for whom it is reserved;\n"
       "the blackness and darkness forever.\n",
       "Your knees start shakin' and your fingers pop\n"
       "Like a pinch on the neck from Mr. Spock!\n",
       "It's worse than that ... He's dead, Jim\n",
	   "Don't Panic!\n",
	   "Et tu... Brute?\n",
	   "Dog of a Saxon! Take thy lance, and prepare for the death thou hast drawn upon thee!\n",
	   "My Precious!  O my Precious!\n",
	   "What are you doing, Dave...?\n",
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

