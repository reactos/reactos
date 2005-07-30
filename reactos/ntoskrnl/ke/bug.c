/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 *
 * PROGRAMMERS:     Alex Ionescu - Rewrote Bugcheck Routines and implemented Reason Callbacks.
 *                  David Welch (welch@cwcom.net)
 *                  Phillip Susi
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static LIST_ENTRY BugcheckCallbackListHead = {NULL,NULL};
static LIST_ENTRY BugcheckReasonCallbackListHead = {NULL,NULL};
static ULONG InBugCheck;
static PRTL_MESSAGE_RESOURCE_DATA KiBugCodeMessages;
static ULONG KeBugCheckCount = 1;

/* FUNCTIONS *****************************************************************/

VOID
INIT_FUNCTION
KiInitializeBugCheck(VOID)
{
    PRTL_MESSAGE_RESOURCE_DATA BugCheckData;
    LDR_RESOURCE_INFO ResourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    NTSTATUS Status;

    /* Initialize Callbadk Listhead and State */
    InitializeListHead(&BugcheckCallbackListHead);
    InitializeListHead(&BugcheckReasonCallbackListHead);
    InBugCheck = 0;

    /* Cache the Bugcheck Message Strings. Prepare the Lookup Data */
    ResourceInfo.Type = 11;
    ResourceInfo.Name = 1;
    ResourceInfo.Language = 9;

    /* Do the lookup. */
    Status = LdrFindResource_U((PVOID)KERNEL_BASE,
                               &ResourceInfo,
                               RESOURCE_DATA_LEVEL,
                               &ResourceDataEntry);

    /* Make sure it worked */
    if (NT_SUCCESS(Status)) {

        DPRINT("Found Bugcheck Resource Data!\n");

        /* Now actually get a pointer to it */
        Status = LdrAccessResource((PVOID)KERNEL_BASE,
                                   ResourceDataEntry,
                                   (PVOID*)&BugCheckData,
                                   NULL);

        /* Make sure it worked */
        if (NT_SUCCESS(Status)) {

            DPRINT("Got Pointer to Bugcheck Resource Data!\n");
            KiBugCodeMessages = BugCheckData;
        }
    }
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeDeregisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State */
    if (CallbackRecord->State == BufferInserted) {

        /* Reset state and remove from list */
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);

        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeDeregisterBugCheckReasonCallback(IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State */
    if (CallbackRecord->State == BufferInserted) {

        /* Reset state and remove from list */
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);

        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeRegisterBugCheckCallback(PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
                           PKBUGCHECK_CALLBACK_ROUTINE	CallbackRoutine,
                           PVOID Buffer,
                           ULONG Length,
                           PUCHAR Component)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State first so we don't double-register */
    if (CallbackRecord->State == BufferEmpty) {

        /* Set the Callback Settings and insert into the list */
        CallbackRecord->Length = Length;
        CallbackRecord->Buffer = Buffer;
        CallbackRecord->Component = Component;
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->State = BufferInserted;
        InsertTailList(&BugcheckCallbackListHead, &CallbackRecord->Entry);

        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeRegisterBugCheckReasonCallback(IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
                                 IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
                                 IN KBUGCHECK_CALLBACK_REASON Reason,
                                 IN PUCHAR Component)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State first so we don't double-register */
    if (CallbackRecord->State == BufferEmpty) {

        /* Set the Callback Settings and insert into the list */
        CallbackRecord->Component = Component;
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->State = BufferInserted;
        CallbackRecord->Reason = Reason;
        InsertTailList(&BugcheckReasonCallbackListHead, &CallbackRecord->Entry);

        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

VOID
STDCALL
KeGetBugMessageText(ULONG BugCheckCode, PANSI_STRING OutputString)
{
    ULONG i;
    ULONG IdOffset;
    ULONG_PTR MessageEntry;
    PCHAR BugCode;

    /* Find the message. This code is based on RtlFindMesssage -- Alex */
    for (i = 0; i < KiBugCodeMessages->NumberOfBlocks; i++)  
    {
        /* Check if the ID Matches */
        if ((BugCheckCode >= KiBugCodeMessages->Blocks[i].LowId) &&
            (BugCheckCode <= KiBugCodeMessages->Blocks[i].HighId)) 
            {
            /* Get Offset to Entry */
            MessageEntry = (ULONG_PTR)KiBugCodeMessages + KiBugCodeMessages->Blocks[i].OffsetToEntries;
            IdOffset = BugCheckCode - KiBugCodeMessages->Blocks[i].LowId;

            /* Get offset to ID */
            for (i = 0; i < IdOffset; i++)
            {
                /* Advance in the Entries */
                MessageEntry += ((PRTL_MESSAGE_RESOURCE_ENTRY)MessageEntry)->Length;
            }

            /* Get the final Code */
            BugCode = ((PRTL_MESSAGE_RESOURCE_ENTRY)MessageEntry)->Text;

            /* Return it in the OutputString */
            if (OutputString) 
            {
                OutputString->Buffer = BugCode;
                OutputString->Length = strlen(BugCode) + 1;
                OutputString->MaximumLength = strlen(BugCode) + 1;
            }
            else 
            {
                /* Direct Output to Screen */
                CHAR BugString[100];
                sprintf(BugString, "%s\n", BugCode);
                InbvDisplayString(BugString);
                break;
            }
        }
    }
}

VOID
STDCALL
KiDoBugCheckCallbacks(VOID)
{
    PKBUGCHECK_CALLBACK_RECORD CurrentRecord;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;

    /* FIXME: Check Checksum and add support for WithReason Callbacks */

    /* First make sure that the list is Initialized... it might not be */
    ListHead = &BugcheckCallbackListHead;
    if (ListHead->Flink && ListHead->Blink) {

        /* Loop the list */
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {

            /* Get the Callback Record */
            CurrentRecord = CONTAINING_RECORD(NextEntry,
                                              KBUGCHECK_CALLBACK_RECORD,
                                              Entry);

            /* Make sure it's inserted */
            if (CurrentRecord->State == BufferInserted) {

                /* Call the routine */
                CurrentRecord->State = BufferStarted;
                (CurrentRecord->CallbackRoutine)(CurrentRecord->Buffer,
                                                 CurrentRecord->Length);
                CurrentRecord->State = BufferFinished;
            }

            /* Move to next Entry */
            NextEntry = NextEntry->Flink;
        }
    }
}

VOID
STDCALL
KeBugCheckWithTf(ULONG BugCheckCode,
                 ULONG BugCheckParameter1,
                 ULONG BugCheckParameter2,
                 ULONG BugCheckParameter3,
                 ULONG BugCheckParameter4,
                 PKTRAP_FRAME Tf)
{
    KIRQL OldIrql;
    BOOLEAN GotExtendedCrashInfo = FALSE;
    PVOID Address = 0;
    PLIST_ENTRY CurrentEntry;
    PLDR_DATA_TABLE_ENTRY CurrentModule = NULL;
    extern LIST_ENTRY ModuleListHead;
#if 0
    CHAR PrintString[100];
#endif
    /* Make sure we're switching back to the blue screen and print messages on it */
    HalReleaseDisplayOwnership();
    if (!KdpDebugMode.Screen)
    {
       /* Enable screen debug mode */
       KdpDebugMode.Screen = TRUE;
       InitRoutines[0](&DispatchTable[0], 0);
    }

    /* Try to find out who did this. For this, we need a Trap Frame.
     * Note: Some special BSODs pass the Frame/EIP as a Param. MSDN has the
     * info so it eventually needs to be supported.
     */
    if (Tf) 
    {
        /* For now, get Address from EIP */
        Address = (PVOID)Tf->Eip;

        /* Try to get information on the module */
        CurrentEntry = ModuleListHead.Flink;
        while (CurrentEntry != &ModuleListHead) 
        {
            /* Get the current Section */
            CurrentModule = CONTAINING_RECORD(CurrentEntry,
                                              LDR_DATA_TABLE_ENTRY,
                                              InLoadOrderModuleList);

            /* Check if this is the right one */
            if ((Address != NULL && (Address >= (PVOID)CurrentModule->DllBase &&
                 Address < (PVOID)((ULONG_PTR)CurrentModule->DllBase + CurrentModule->SizeOfImage)))) 
            {
                /* We got it */
                GotExtendedCrashInfo = TRUE;
                break;
            }

            /* Loop again */
            CurrentEntry = CurrentEntry->Flink;
        }
    }

    /* Raise IRQL to HIGH_LEVEL */
    Ke386DisableInterrupts();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Unload the Kernel Adress Space if we own it */
    if (MmGetKernelAddressSpace()->Lock.Owner == KeGetCurrentThread())
        MmUnlockAddressSpace(MmGetKernelAddressSpace());

    /* FIXMEs: Use inbv to clear, fill and write to screen. */

    /* Show the STOP Message */
#if 0
    InbvDisplayString("A problem has been detected and ReactOS has been shut down to prevent "
                      "damage to your computer.\n\n");
#else
    DbgPrint("A problem has been detected and ReactOS has been shut down to prevent "
             "damage to your computer.\n\n");
#endif
    /* Show the module name of who caused this */
    if (GotExtendedCrashInfo) 
    {
#if 0
        sprintf(PrintString, 
                "The problem seems to be caused by the following file: %wZ\n\n",
                &CurrentModule->BaseDllName);
        InbvDisplayString(PrintString);
#else
        DbgPrint("The problem seems to be caused by the following file: %wZ\n\n",
                 &CurrentModule->BaseDllName);
#endif
    }

    /* Find the Bug Code String */
    KeGetBugMessageText(BugCheckCode, NULL);

    /* Show the techincal Data */
#if 0
    sprintf(PrintString,
            "Technical information:\n\n*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)\n\n",
            BugCheckCode,
            (PVOID)BugCheckParameter1,
            (PVOID)BugCheckParameter2,
            (PVOID)BugCheckParameter3,
            (PVOID)BugCheckParameter4);
    InbvDisplayString(PrintString);
#else
    DbgPrint("Technical information:\n\n*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)\n\n",
             BugCheckCode,
            (PVOID)BugCheckParameter1,
            (PVOID)BugCheckParameter2,
            (PVOID)BugCheckParameter3,
            (PVOID)BugCheckParameter4);
#endif
    /* Show the module name and more data of who caused this */
    if (GotExtendedCrashInfo) 
    {
#if 0
        sprintf(PrintString,
                "***    %wZ - Address 0x%p base at 0x%p, DateStamp 0x%x\n\n",
                &CurrentModule->BaseDllName,
                Address,
                (PVOID)CurrentModule->DllBase,
                0);
        InbvDisplayString(PrintString);
#else
        DbgPrint("***    %wZ - Address 0x%p base at 0x%p, DateStamp 0x%x\n\n",
                 &CurrentModule->BaseDllName,
                 Address,
                 (PVOID)CurrentModule->DllBase,
                 0);
#endif
    }

    /* There can only be one Bugcheck per Bootup */
    if (!InterlockedDecrement((PLONG)&KeBugCheckCount)) 
    {
#ifdef CONFIG_SMP
        LONG i;
        /* Freeze the other CPUs */
        for (i = 0; i < KeNumberProcessors; i++) 
        {
            if (i != (LONG)KeGetCurrentProcessorNumber())
            {
                /* Send the IPI and give them one second to catch up */
                KiIpiSendRequest(1 << i, IPI_REQUEST_FREEZE);
                KeStallExecutionProcessor(1000000);
            }
        }
#endif
        /* Check if we got a Trap Frame */
        if (Tf) 
        {
            /* Dump it */
            KiDumpTrapFrame(Tf, BugCheckParameter1, BugCheckParameter2);
        } 
        else 
        {
            /* We can only dump the frames */
#if defined(__GNUC__)
            KeDumpStackFrames((PULONG)__builtin_frame_address(0));
#elif defined(_MSC_VER)
            __asm push ebp
            __asm call KeDumpStackFrames
            __asm add esp, 4
#else
#error Unknown compiler for inline assembler
#endif
        }

        /* Call the Callbacks */;
        KiDoBugCheckCallbacks();

        /* Dump the BSOD to the Paging File */
        MmDumpToPagingFile(BugCheckCode,
                           BugCheckParameter1,
                           BugCheckParameter2,
                           BugCheckParameter3,
                           BugCheckParameter4,
                           Tf);

        /* Wake up the Debugger */
        if (KdDebuggerEnabled) 
        {
            Ke386EnableInterrupts();
            DbgBreakPointWithStatus(DBG_STATUS_BUGCHECK_SECOND);
            Ke386DisableInterrupts();
        }
    }

    /* Halt this CPU now */
    for (;;) Ke386HaltProcessor();
}

/*
 * @implemented
 *
 * FUNCTION: Brings the system down in a controlled manner when an
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 *           BugCheckParameter[1-4] = Additional information about bug
 * RETURNS: Doesn't
 */
VOID
STDCALL
KeBugCheckEx(ULONG BugCheckCode,
             ULONG BugCheckParameter1,
             ULONG BugCheckParameter2,
             ULONG BugCheckParameter3,
             ULONG BugCheckParameter4)
{
    /* Call the Trap Frame version without a Trap Frame */
    KeBugCheckWithTf(BugCheckCode,
                     BugCheckParameter1,
                     BugCheckParameter2,
                     BugCheckParameter3,
                     BugCheckParameter4,
                     NULL);
}

/*
 * @implemented
 *
 * FUNCTION: Brings the system down in a controlled manner when an
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 * RETURNS: Doesn't
 */
VOID
STDCALL
KeBugCheck(ULONG BugCheckCode)
{
    KeBugCheckEx(BugCheckCode, 0, 0, 0, 0);
}

/* EOF */
