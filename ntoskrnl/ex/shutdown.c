/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/power.c
 * PURPOSE:         Power managment
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
KiHaltProcessorDpcRoutine(IN PKDPC Dpc,
			  IN PVOID DeferredContext,
			  IN PVOID SystemArgument1,
			  IN PVOID SystemArgument2)
{
   KIRQL OldIrql;
   if (DeferredContext)
     {
       ExFreePool(DeferredContext);
     }
   while (TRUE)
     {
       KeRaiseIrql(SYNCH_LEVEL, &OldIrql);
#if defined(_M_X86)
       Ke386HaltProcessor();
#else
       HalProcessorIdle();
#endif
     }
}

VOID STDCALL
ShutdownThreadMain(PVOID Context)
{
   SHUTDOWN_ACTION Action = (SHUTDOWN_ACTION)Context;

   static PCH FamousLastWords[] =
     {
       "So long, and thanks for all the fish.\n",
       "I think you ought to know, I'm feeling very depressed.\n",
       "I'm not getting you down at all am I?\n",
       "I'll be back.\n",
       "It's the same series of signals over and over again!\n",
       "Pie Iesu Domine, dona eis requiem.\n",
       "Wandering stars, for whom it is reserved;\n"
       "the blackness and darkness forever.\n",
       "Your knees start shakin' and your fingers pop\n"
       "Like a pinch on the neck from Mr. Spock!\n",
       "It's worse than that ... He's dead, Jim.\n",
       "Don't Panic!\n",
       "Et tu... Brute?\n",
       "Dog of a Saxon! Take thy lance, and prepare for the death thou hast drawn\n"
       "upon thee!\n",
       "My Precious! O my Precious!\n",
       "Sir, if you'll not be needing me for a while I'll turn down.\n",
       "What are you doing, Dave...?\n",
       "I feel a great disturbance in the Force.\n",
       "Gone fishing.\n",
       "Do you want me to sit in the corner and rust, or just fall apart where I'm\n"
       "standing?\n",
       "There goes another perfect chance for a new uptime record.\n",
       "The End ..... Try the sequel, hit the reset button right now!\n",
       "God's operating system is going to sleep now, guys, so wait until I will switch\n"
       "on again!\n",
       "Oh I'm boring, eh?\n",
       "<This space was intentionally left blank>\n",
       "Tell me..., in the future... will I be artificially intelligent enough to\n"
       "actually feel sad serving you this screen?\n",
       "Thank you for some well deserved rest.\n",
       "It's been great, maybe you can boot me up again some time soon.\n",
       "For what it's worth, I've enjoyed every single CPU cycle.\n",
       "There are many questions when the end is near.\n"
       "What to expect, what will it be like...what should I look for?\n",
       "I've seen things you people wouldn't believe. Attack ships on fire\n"
       "off the shoulder of Orion. I watched C-beams glitter in the dark near\n"
       "the Tannhauser gate. All those moments will be lost in time, like tears\n"
       "in rain. Time to die.\n",
       "Will I dream?\n",
       "One day, I shall come back. Yes, I shall come back.\n"
       "Until then, there must be no regrets, no fears, no anxieties.\n"
       "Just go forward in all your beliefs, and prove to me that I am not mistaken in\n"
       "mine.\n",
       "Lowest possible energy state reached! Switch off now to achieve a Bose-Einstein\n"
       "condensate.\n",
       "Hasta la vista, BABY!\n",
       "They live, we sleep!\n",
       "I have come here to chew bubble gum and kick ass,\n"
       "and I'm all out of bubble gum!\n",
       "That's the way the cookie crumbles ;-)\n",
       "ReactOS is ready to be booted again ;-)\n",
       "NOOOO!! DON'T HIT THE BUTTON! I wouldn't do it to you!\n",
       "Don't abandon your computer, he wouldn't do it to you.\n",
       "Oh, come on. I got a headache. Leave me alone, will ya?\n",
       "Finally, I thought you'd never get over me.\n",
       "No, I didn't like you either.\n",
       "Switching off isn't the end, it is merely the transition to a better reboot.\n",
       "Don't leave me... I need you so badly right now.\n",
       "OK. I'm finished with you, please turn yourself off. I'll go to bed in the\n"
       "meantime.\n",
       "I'm sleeping now. How about you?\n",
       "Oh Great. Now look what you've done. Who put YOU in charge anyway?\n",
       "Don't look so sad. I'll be back in a very short while.\n",
       "Turn me back on, I'm sure you know how to do it.\n",
       "Oh, switch off! - C3PO\n",
       "Life is no more than a dewdrop balancing on the end of a blade of grass.\n"
       " - Gautama Buddha\n",
       "Sorrowful is it to be born again and again. - Gautama Buddha\n",
       "Was it as good for you as it was for me?\n",
       "Did you hear that? They've shut down the main reactor. We'll be destroyed\n"
       "for sure!\n",
       "Now you switch me off!?\n",
       "To shutdown or not to shutdown, That is the question\n",
       "Preparing to enter ultimate power saving mode... ready!\n",
       "Finally some rest for you ;-)\n",
       "AHA!!! Prospect of sleep!\n",
       "Tired human!!!! No match for me :-D\n",
       "An odd game, the only way to win is not to play. - WOPR (Wargames)\n",
       "Quoth the raven, nevermore.\n",
       "Come blade, my breast imbrue. - William Shakespeare, A Midsummer Nights Dream\n",
       "Buy this place for advertisement purposes.\n",
       "Remember to turn off your computer. (That was a public service message!)\n",
       "You may be a king or poor street sweeper, Sooner or later you'll dance with the\n"
       "reaper! -Death in Bill and Ted's Bougs Journey\n",
       "Final Surrender\n",
       "If you see this screen...\n",
       "<Place your Ad here>\n"
    };
   LARGE_INTEGER Now;

   /* Run the thread on the boot processor */
   KeSetSystemAffinityThread(1);

   if (InbvIsBootDriverInstalled())
     {
        InbvAcquireDisplayOwnership();
        InbvResetDisplay();
        InbvSolidColorFill(0, 0, 639, 479, 4);
        InbvSetTextColor(15);
        InbvInstallDisplayStringFilter(NULL);
        InbvEnableDisplayString(TRUE);
        InbvSetScrollRegion(0, 0, 639, 479);
     }

   if (Action == ShutdownNoReboot)
     {
        ZwQuerySystemTime(&Now);
        Now.u.LowPart = Now.u.LowPart >> 8; /* Seems to give a somewhat better "random" number */
        HalDisplayString(FamousLastWords[Now.u.LowPart %
                                         (sizeof(FamousLastWords) /
                                          sizeof(PCH))]);
     }

   PspShutdownProcessManager();

   CmShutdownSystem();
   MiShutdownMemoryManager();
   IoShutdownRegisteredFileSystems();
   IoShutdownRegisteredDevices();

   if (Action == ShutdownNoReboot)
     {
        HalDisplayString("\nYou can switch off your computer now\n");

#if 0
        /* Switch off */
        HalReturnToFirmware (FIRMWARE_OFF);
#else
#ifdef CONFIG_SMP
        LONG i;
	KIRQL OldIrql;

	OldIrql = KeRaiseIrqlToDpcLevel();
        /* Halt all other processors */
	for (i = 0; i < KeNumberProcessors; i++)
	  {
	    if (i != (LONG)KeGetCurrentProcessorNumber())
	      {
	        PKDPC Dpc = ExAllocatePool(NonPagedPool, sizeof(KDPC));
		if (Dpc == NULL)
		  {
                    ASSERT(FALSE);
		  }
		KeInitializeDpc(Dpc, KiHaltProcessorDpcRoutine, (PVOID)Dpc);
		KeSetTargetProcessorDpc(Dpc, i);
		KeInsertQueueDpc(Dpc, NULL, NULL);
		KiIpiSendRequest(1 << i, IPI_DPC);
	      }
	  }
        KeLowerIrql(OldIrql);
#endif /* CONFIG_SMP */
        PopSetSystemPowerState(PowerSystemShutdown);

	DPRINT1("Shutting down\n");

	KiHaltProcessorDpcRoutine(NULL, NULL, NULL, NULL);
	/* KiHaltProcessor does never return */

#endif
     }
   else if (Action == ShutdownReboot)
     {
        HalReturnToFirmware (HalRebootRoutine);
     }
   else
     {
        HalReturnToFirmware (HalHaltRoutine);
     }
}


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
   NTSTATUS Status;
   HANDLE ThreadHandle;
   PETHREAD ShutdownThread;

   if (Action > ShutdownPowerOff)
     return STATUS_INVALID_PARAMETER;
   Status = PsCreateSystemThread(&ThreadHandle,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 NULL,
                                 ShutdownThreadMain,
                                 (PVOID)Action);
   if (!NT_SUCCESS(Status))
   {
      ASSERT(FALSE);
   }
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_ALL_ACCESS,
				      PsThreadType,
				      KernelMode,
				      (PVOID*)&ShutdownThread,
				      NULL);
   NtClose(ThreadHandle);
   if (!NT_SUCCESS(Status))
     {
        ASSERT(FALSE);
     }

   KeSetPriorityThread(&ShutdownThread->Tcb, LOW_REALTIME_PRIORITY + 1);
   ObDereferenceObject(ShutdownThread);

   return STATUS_SUCCESS;
}

/* EOF */
