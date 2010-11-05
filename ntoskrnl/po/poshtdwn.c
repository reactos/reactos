/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/po/poshtdwn.c
 * PURPOSE:         Power Manager Shutdown Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG PopShutdownPowerOffPolicy;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
PopShutdownHandler(VOID)
{
    PUCHAR Logo1, Logo2;
    ULONG i;
    
    /* Stop all interrupts */
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

VOID
NTAPI
PopShutdownSystem(IN POWER_ACTION SystemAction)
{
    /* Note should notify caller of NtPowerInformation(PowerShutdownNotification) */

    /* Unload symbols */
    DPRINT1("It's the final countdown...%lx\n", SystemAction);
    DbgUnLoadImageSymbols(NULL, (PVOID)-1, 0);
    
    /* Run the thread on the boot processor */
    KeSetSystemAffinityThread(1);

    /* Now check what the caller wants */
    switch (SystemAction)
    {
        /* Reset */
        case PowerActionShutdownReset:

            /* Try platform driver first, then legacy */
            //PopInvokeSystemStateHandler(PowerStateShutdownReset, NULL);
            HalReturnToFirmware(HalRebootRoutine);
            break;

        case PowerActionShutdown:

            /* Check for group policy that says to use "it is now safe" screen */
            if (PopShutdownPowerOffPolicy)
            {
                /* FIXFIX: Switch to legacy shutdown handler */
                //PopPowerStateHandlers[PowerStateShutdownOff].Handler = PopShutdownHandler;
            }

        case PowerActionShutdownOff:

            /* Call shutdown handler */
            //PopInvokeSystemStateHandler(PowerStateShutdownOff, NULL);
            
            /* ReactOS Hack */
            PopSetSystemPowerState(PowerSystemShutdown);
            PopShutdownHandler();

            /* If that didn't work, call the HAL */
            HalReturnToFirmware(HalPowerDownRoutine);
            break;

        default:
            break;
    }

    /* Anything else should not happen */
    KeBugCheckEx(INTERNAL_POWER_ERROR, 5, 0, 0, 0);
}

VOID
NTAPI
PopGracefulShutdown(IN PVOID Context)
{
    PEPROCESS Process = NULL;

    /* Loop every process */
    Process = PsGetNextProcess(Process);
    while (Process)
    {
        /* Make sure this isn't the idle or initial process */
        if ((Process != PsInitialSystemProcess) && (Process != PsIdleProcess))
        {
            /* Print it */
            DPRINT1("%15s is still RUNNING (%lx)\n", Process->ImageFileName, Process->UniqueProcessId);
        }

        /* Get the next process */
        Process = PsGetNextProcess(Process);
    }
    
    /* First, the HAL handles any "end of boot" special functionality */
    DPRINT1("HAL shutting down\n");
    HalEndOfBoot();

    /* In this step, the I/O manager does first-chance shutdown notification */
    DPRINT1("I/O manager shutting down in phase 0\n");    
    IoShutdownSystem(0);
    
    /* In this step, all workers are killed and hives are flushed */
    DPRINT1("Configuration Manager shutting down\n");
    CmShutdownSystem();
    
    /* Note that modified pages should be written here (MiShutdownSystem) */

    /* In this step, the I/O manager does last-chance shutdown notification */
    DPRINT1("I/O manager shutting down in phase 1\n"); 
    IoShutdownSystem(1);
    CcWaitForCurrentLazyWriterActivity();

    /* Note that here, we should broadcast the power IRP to devices */

    /* In this step, the HAL disables any wake timers */
    DPRINT1("Disabling wake timers\n");
    HalSetWakeEnable(FALSE);
    
    /* And finally the power request is sent */
    DPRINT1("Taking the system down\n");
    PopShutdownSystem(PopAction.Action);
}

VOID
NTAPI
PopReadShutdownPolicy(VOID)
{
    UNICODE_STRING KeyString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE KeyHandle;
    ULONG Length;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION Info = (PVOID)Buffer;
    
    /* Setup object attributes */
    RtlInitUnicodeString(&KeyString,
                         L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\Windows NT");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open the key */
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Open the policy value and query it */
        RtlInitUnicodeString(&KeyString, L"DontPowerOffAfterShutdown");
        Status = ZwQueryValueKey(KeyHandle,
                                 &KeyString,
                                 KeyValuePartialInformation,
                                 &Info,
                                 sizeof(Info),
                                 &Length);
        if ((NT_SUCCESS(Status)) && (Info->Type == REG_DWORD))
        {
            /* Read the policy */
            PopShutdownPowerOffPolicy = *Info->Data == 1;
        }

        /* Close the key */
        ZwClose(KeyHandle);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

