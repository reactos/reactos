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

VOID NTAPI
ShutdownThreadMain(PVOID Context)
{
   SHUTDOWN_ACTION Action = (SHUTDOWN_ACTION)Context;
   PUCHAR Logo1, Logo2;
   ULONG i;

   /* Run the thread on the boot processor */
   KeSetSystemAffinityThread(1);

   PspShutdownProcessManager();

   CmShutdownSystem();
   IoShutdownRegisteredFileSystems();
   IoShutdownRegisteredDevices();
    
   if (Action == ShutdownNoReboot)
     {
        /* Try the platform driver */
        PopSetSystemPowerState(PowerSystemShutdown);
        
        /* If that didn't work, try legacy switch off */
        //HalReturnToFirmware(HalPowerDownRoutine);
        
        /* If that still didn't work, stop all interrupts */
        KeRaiseIrqlToDpcLevel();
        _disable();

        /* Do we have boot video */
        if (InbvIsBootDriverInstalled())
        {
            /* Yes we do, cleanup for shutdown screen */
            if (!InbvCheckDisplayOwnership()) InbvAcquireDisplayOwnership();
            InbvResetDisplay();
            InbvSolidColorFill(0, 0, 639, 479, 0);
            InbvEnableDisplayString(TRUE);
            InbvSetScrollRegion(0, 0, 639, 479);

            /* Display shutdown logo and message */
            Logo1 = InbvGetResourceAddress(IDB_SHUTDOWN_LOGO);
            Logo2 = InbvGetResourceAddress(IDB_LOGO);
            if ((Logo1) && (Logo2))
            {
                InbvBitBlt(Logo1, 215, 352);
                InbvBitBlt(Logo2, 217, 111);
            }
        }
        else
        {
            /* Do it in text-mode */
            for (i = 0; i < 25; i++) InbvDisplayString("\n");
            InbvDisplayString("                       ");
            InbvDisplayString("The system may be powered off now.\n");
        }

        /* Hang the system */
        for (;;) HalHaltSystem();
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


NTSTATUS NTAPI
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
NTSTATUS NTAPI
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
