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
       "Sir, If you'll not be needing me for a while I'll turn down.\n",
       "What are you doing, Dave...?\n",
       "I feel a great disturbance in the Force\n",
       "Gone fishing\n",
       "Do you want me to sit in the corner and rust, or just fall apart where I'm standing?\n",
       "There goes another perfect chance for a new uptime record\n",
       "The end ..... Try the sequel, hit the reset button right now!\n",
       "God's operating system is going to sleep now, guys, so wait until I will switch on again!\n",
       "Oh i'm boring eh?\n",
       "<This space was intentionally left blank>\n",
       "tell me..., in the future... will I be artificial intelligent enough to actually feel\n"
       "sad serving you this screen?\n",
       "Thank you for some well deserved rest.\n",
       "It’s been great, maybe we can boot me up again some time soon.\n",
       "For what’s it worth, I’ve enjoyed every single CPU cycle.\n",
       "There are many questions when the end is near.\n"
       "What to expect, what will it be like...what should I look for?\n",
       "I've seen things you people wouldn't believe. Attack ships on fire\n"
       "off the shoulder of Orion. I watched C-beams glitter in the dark near\n"
       "the Tannhauser gate. All those moments will be lost in time, like tears\n"
       "in rain. Time to die.\n",
       "Will I dream?\n",
       "One day, I shall come back. Yes, I shall come back.\n"
       "Until then, there must be no regrets, no fears, no anxieties.\n"
       "Just go forward in all your beliefs, and prove to me that I am not mistaken in mine.\n",
       "Lowest possible energy state reached! Switch off now to achive a Bose-Einstein condensate.\n",
       "Hasta la vista, BABY!\n"
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
        HalDisplayString("\nYou can switch off your computer now\n\n");
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

