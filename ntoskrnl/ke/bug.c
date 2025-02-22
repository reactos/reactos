/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Bugcheck Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#ifdef KDBG
#include <kdbg/kdb.h>
#endif

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY KeBugcheckCallbackListHead;
LIST_ENTRY KeBugcheckReasonCallbackListHead;
KSPIN_LOCK BugCheckCallbackLock;
ULONG KeBugCheckActive, KeBugCheckOwner;
LONG KeBugCheckOwnerRecursionCount;
PMESSAGE_RESOURCE_DATA KiBugCodeMessages;
ULONG KeBugCheckCount = 1;
ULONG KiHardwareTrigger;
PUNICODE_STRING KiBugCheckDriver;
ULONG_PTR KiBugCheckData[5];

PKNMI_HANDLER_CALLBACK KiNmiCallbackListHead = NULL;
KSPIN_LOCK KiNmiCallbackListLock;

/* Jira Reporting */
UNICODE_STRING KeRosProcessorName, KeRosBiosDate, KeRosBiosVersion;
UNICODE_STRING KeRosVideoBiosDate, KeRosVideoBiosVersion;

/* PRIVATE FUNCTIONS *********************************************************/

PVOID
NTAPI
KiPcToFileHeader(IN PVOID Pc,
                 OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                 IN BOOLEAN DriversOnly,
                 OUT PBOOLEAN InKernel)
{
    ULONG i = 0;
    PVOID ImageBase, PcBase = NULL;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY ListHead, NextEntry;

    /* Check which list we should use */
    ListHead = (KeLoaderBlock) ? &KeLoaderBlock->LoadOrderListHead :
                                 &PsLoadedModuleList;

    /* Assume no */
    *InKernel = FALSE;

    /* Set list pointers and make sure it's valid */
    NextEntry = ListHead->Flink;
    if (NextEntry)
    {
        /* Start loop */
        while (NextEntry != ListHead)
        {
            /* Increase entry */
            i++;

            /* Check if this is a kernel entry and we only want drivers */
            if ((i <= 2) && (DriversOnly != FALSE))
            {
                /* Skip it */
                NextEntry = NextEntry->Flink;
                continue;
            }

            /* Get the loader entry */
            Entry = CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            /* Move to the next entry */
            NextEntry = NextEntry->Flink;
            ImageBase = Entry->DllBase;

            /* Check if this is the right one */
            if (((ULONG_PTR)Pc >= (ULONG_PTR)Entry->DllBase) &&
                ((ULONG_PTR)Pc < ((ULONG_PTR)Entry->DllBase + Entry->SizeOfImage)))
            {
                /* Return this entry */
                *LdrEntry = Entry;
                PcBase = ImageBase;

                /* Check if this was a kernel or HAL entry */
                if (i <= 2) *InKernel = TRUE;
                break;
            }
        }
    }

    /* Return the base address */
    return PcBase;
}

PVOID
NTAPI
KiRosPcToUserFileHeader(IN PVOID Pc,
                        OUT PLDR_DATA_TABLE_ENTRY *LdrEntry)
{
    PVOID ImageBase, PcBase = NULL;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY ListHead, NextEntry;

    /*
     * We know this is valid because we should only be called after a
     * succesfull address from RtlWalkFrameChain for UserMode, which
     * validates everything for us.
     */
    ListHead = &KeGetCurrentThread()->
               Teb->ProcessEnvironmentBlock->Ldr->InLoadOrderModuleList;

    /* Set list pointers and make sure it's valid */
    NextEntry = ListHead->Flink;
    if (NextEntry)
    {
        /* Start loop */
        while (NextEntry != ListHead)
        {
            /* Get the loader entry */
            Entry = CONTAINING_RECORD(NextEntry,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            /* Move to the next entry */
            NextEntry = NextEntry->Flink;
            ImageBase = Entry->DllBase;

            /* Check if this is the right one */
            if (((ULONG_PTR)Pc >= (ULONG_PTR)Entry->DllBase) &&
                ((ULONG_PTR)Pc < ((ULONG_PTR)Entry->DllBase + Entry->SizeOfImage)))
            {
                /* Return this entry */
                *LdrEntry = Entry;
                PcBase = ImageBase;
                break;
            }
        }
    }

    /* Return the base address */
    return PcBase;
}

USHORT
NTAPI
KeRosCaptureUserStackBackTrace(IN ULONG FramesToSkip,
                               IN ULONG FramesToCapture,
                               OUT PVOID *BackTrace,
                               OUT PULONG BackTraceHash OPTIONAL)
{
    PVOID Frames[2 * 64];
    ULONG FrameCount;
    ULONG Hash = 0, i;

    /* Skip a frame for the caller */
    FramesToSkip++;

    /* Don't go past the limit */
    if ((FramesToCapture + FramesToSkip) >= 128) return 0;

    /* Do the back trace */
    FrameCount = RtlWalkFrameChain(Frames, FramesToCapture + FramesToSkip, 1);

    /* Make sure we're not skipping all of them */
    if (FrameCount <= FramesToSkip) return 0;

    /* Loop all the frames */
    for (i = 0; i < FramesToCapture; i++)
    {
        /* Don't go past the limit */
        if ((FramesToSkip + i) >= FrameCount) break;

        /* Save this entry and hash it */
        BackTrace[i] = Frames[FramesToSkip + i];
        Hash += PtrToUlong(BackTrace[i]);
    }

    /* Write the hash */
    if (BackTraceHash) *BackTraceHash = Hash;

    /* Clear the other entries and return count */
    RtlFillMemoryUlong(Frames, 128, 0);
    return (USHORT)i;
}


VOID
FASTCALL
KeRosDumpStackFrameArray(IN PULONG_PTR Frames,
                         IN ULONG FrameCount)
{
    ULONG i;
    ULONG_PTR Addr;
    BOOLEAN InSystem;
    PVOID p;

    /* GCC complaints that it may be used uninitialized */
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;

    /* Loop them */
    for (i = 0; i < FrameCount; i++)
    {
        /* Get the EIP */
        Addr = Frames[i];
        if (!Addr)
        {
        	break;
        }

        /* Get the base for this file */
        if (Addr > (ULONG_PTR)MmHighestUserAddress)
        {
            /* We are in kernel */
            p = KiPcToFileHeader((PVOID)Addr, &LdrEntry, FALSE, &InSystem);
        }
        else
        {
            /* We are in user land */
            p = KiRosPcToUserFileHeader((PVOID)Addr, &LdrEntry);
        }
        if (p)
        {
#ifdef KDBG
            if (!KdbSymPrintAddress((PVOID)Addr, NULL))
#endif
            {
                CHAR AnsiName[64];

                /* Convert module name to ANSI and print it */
                KeBugCheckUnicodeToAnsi(&LdrEntry->BaseDllName,
                                        AnsiName,
                                        sizeof(AnsiName));
                Addr -= (ULONG_PTR)LdrEntry->DllBase;
                DbgPrint("<%s: %p>", AnsiName, (PVOID)Addr);
            }
        }
        else
        {
            /* Print only the address */
            DbgPrint("<%p>", (PVOID)Addr);
        }

        /* Go to the next frame */
        DbgPrint("\n");
    }
}

VOID
NTAPI
KeRosDumpStackFrames(IN PULONG_PTR Frame OPTIONAL,
                     IN ULONG FrameCount OPTIONAL)
{
    ULONG_PTR Frames[32];
    ULONG RealFrameCount;

    /* If the caller didn't ask, assume 32 frames */
    if (!FrameCount || FrameCount > 32) FrameCount = 32;

    if (Frame)
    {
        /* Dump them */
        KeRosDumpStackFrameArray(Frame, FrameCount);
    }
    else
    {
        /* Get the current frames (skip the two. One for the dumper, one for the caller) */
        RealFrameCount = RtlCaptureStackBackTrace(2, FrameCount, (PVOID*)Frames, NULL);
        DPRINT1("RealFrameCount =%lu\n", RealFrameCount);

        /* Dump them */
        KeRosDumpStackFrameArray(Frames, RealFrameCount);

        /* Count left for user mode? */
        if (FrameCount - RealFrameCount > 0)
        {
            /* Get the current frames */
            RealFrameCount = KeRosCaptureUserStackBackTrace(-1, FrameCount - RealFrameCount, (PVOID*)Frames, NULL);

            /* Dump them */
            KeRosDumpStackFrameArray(Frames, RealFrameCount);
        }
    }
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeBugCheck(VOID)
{
    PMESSAGE_RESOURCE_DATA BugCheckData;
    LDR_RESOURCE_INFO ResourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    /* Get the kernel entry */
    LdrEntry = CONTAINING_RECORD(KeLoaderBlock->LoadOrderListHead.Flink,
                                 LDR_DATA_TABLE_ENTRY,
                                 InLoadOrderLinks);

    /* Cache the Bugcheck Message Strings. Prepare the Lookup Data */
    ResourceInfo.Type = 11;
    ResourceInfo.Name = 1;
    ResourceInfo.Language = 9;

    /* Do the lookup. */
    Status = LdrFindResource_U(LdrEntry->DllBase,
                               &ResourceInfo,
                               RESOURCE_DATA_LEVEL,
                               &ResourceDataEntry);

    /* Make sure it worked */
    if (NT_SUCCESS(Status))
    {
        /* Now actually get a pointer to it */
        Status = LdrAccessResource(LdrEntry->DllBase,
                                   ResourceDataEntry,
                                   (PVOID*)&BugCheckData,
                                   NULL);
        if (NT_SUCCESS(Status)) KiBugCodeMessages = BugCheckData;
    }
}

BOOLEAN
NTAPI
KeGetBugMessageText(IN ULONG BugCheckCode,
                    OUT PANSI_STRING OutputString OPTIONAL)
{
    ULONG i;
    ULONG IdOffset;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PCHAR BugCode;
    USHORT Length;
    BOOLEAN Result = FALSE;

    /* Make sure we're not bugchecking too early */
    if (!KiBugCodeMessages) return Result;

    /*
     * Globally protect in SEH as we are trying to access data in
     * dire situations, and potentially going to patch it (see below).
     */
    _SEH2_TRY
    {

    /*
     * Make the kernel resource section writable, as we are going to manually
     * trim the trailing newlines in the bugcheck resource message in place,
     * when OutputString is NULL and before displaying it on screen.
     */
    MmMakeKernelResourceSectionWritable();

    /* Find the message. This code is based on RtlFindMesssage */
    for (i = 0; i < KiBugCodeMessages->NumberOfBlocks; i++)
    {
        /* Check if the ID matches */
        if ((BugCheckCode >= KiBugCodeMessages->Blocks[i].LowId) &&
            (BugCheckCode <= KiBugCodeMessages->Blocks[i].HighId))
        {
            /* Get offset to entry */
            MessageEntry = (PMESSAGE_RESOURCE_ENTRY)
                ((ULONG_PTR)KiBugCodeMessages + KiBugCodeMessages->Blocks[i].OffsetToEntries);
            IdOffset = BugCheckCode - KiBugCodeMessages->Blocks[i].LowId;

            /* Advance in the entries until finding it */
            while (IdOffset--)
            {
                MessageEntry = (PMESSAGE_RESOURCE_ENTRY)
                    ((ULONG_PTR)MessageEntry + MessageEntry->Length);
            }

            /* Make sure it's not Unicode */
            ASSERT(!(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE));

            /* Get the final code */
            BugCode = (PCHAR)MessageEntry->Text;
            Length = (USHORT)strlen(BugCode);

            /* Handle trailing newlines */
            while ((Length > 0) && ((BugCode[Length - 1] == '\n') ||
                                    (BugCode[Length - 1] == '\r') ||
                                    (BugCode[Length - 1] == ANSI_NULL)))
            {
                /* Directly trim the newline in place if we don't return the string */
                if (!OutputString) BugCode[Length - 1] = ANSI_NULL;

                /* Skip the trailing newline */
                Length--;
            }

            /* Check if caller wants an output string */
            if (OutputString)
            {
                /* Return it in the OutputString */
                OutputString->Buffer = BugCode;
                OutputString->Length = Length;
                OutputString->MaximumLength = Length;
            }
            else
            {
                /* Direct output to screen */
                InbvDisplayString(BugCode);
                InbvDisplayString("\r");
            }

            /* We're done */
            Result = TRUE;
            break;
        }
    }

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    /* Return the result */
    return Result;
}

VOID
NTAPI
KiDoBugCheckCallbacks(VOID)
{
    PKBUGCHECK_CALLBACK_RECORD CurrentRecord;
    PLIST_ENTRY ListHead, NextEntry, LastEntry;
    ULONG_PTR Checksum;

    /* First make sure that the list is initialized... it might not be */
    ListHead = &KeBugcheckCallbackListHead;
    if ((!ListHead->Flink) || (!ListHead->Blink))
        return;

    /* Loop the list */
    LastEntry = ListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the reord */
        CurrentRecord = CONTAINING_RECORD(NextEntry,
                                          KBUGCHECK_CALLBACK_RECORD,
                                          Entry);

        /* Validate it */
        // TODO/FIXME: Check whether the memory CurrentRecord points to
        // is still accessible and valid!
        if (CurrentRecord->Entry.Blink != LastEntry) return;
        Checksum = (ULONG_PTR)CurrentRecord->CallbackRoutine;
        Checksum += (ULONG_PTR)CurrentRecord->Buffer;
        Checksum += (ULONG_PTR)CurrentRecord->Length;
        Checksum += (ULONG_PTR)CurrentRecord->Component;

        /* Make sure it's inserted and validated */
        if ((CurrentRecord->State == BufferInserted) &&
            (CurrentRecord->Checksum == Checksum))
        {
            /* Call the routine */
            CurrentRecord->State = BufferStarted;
            _SEH2_TRY
            {
                (CurrentRecord->CallbackRoutine)(CurrentRecord->Buffer,
                                                 CurrentRecord->Length);
                CurrentRecord->State = BufferFinished;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                CurrentRecord->State = BufferIncomplete;
            }
            _SEH2_END;
        }

        /* Go to the next entry */
        LastEntry = NextEntry;
        NextEntry = NextEntry->Flink;
    }
}

VOID
NTAPI
KiBugCheckDebugBreak(IN ULONG StatusCode)
{
    /*
     * Wrap this in SEH so we don't crash if
     * there is no debugger or if it disconnected
     */
DoBreak:
    _SEH2_TRY
    {
        /* Breakpoint */
        DbgBreakPointWithStatus(StatusCode);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* No debugger, halt the CPU */
        HalHaltSystem();
    }
    _SEH2_END;

    /* Break again if this wasn't first try */
    if (StatusCode != DBG_STATUS_BUGCHECK_FIRST) goto DoBreak;
}

PCHAR
NTAPI
KeBugCheckUnicodeToAnsi(IN PUNICODE_STRING Unicode,
                        OUT PCHAR Ansi,
                        IN ULONG Length)
{
    PCHAR p;
    PWCHAR pw;
    ULONG i;

    /* Set length and normalize it */
    i = Unicode->Length / sizeof(WCHAR);
    i = min(i, Length - 1);

    /* Set source and destination, and copy */
    pw = Unicode->Buffer;
    p = Ansi;
    while (i--) *p++ = (CHAR)*pw++;

    /* Null terminate and return */
    *p = ANSI_NULL;
    return Ansi;
}

VOID
NTAPI
KiDumpParameterImages(IN PCHAR Message,
                      IN PULONG_PTR Parameters,
                      IN ULONG ParameterCount,
                      IN PKE_BUGCHECK_UNICODE_TO_ANSI ConversionRoutine)
{
    ULONG i;
    BOOLEAN InSystem;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PVOID ImageBase;
    PUNICODE_STRING DriverName;
    CHAR AnsiName[32];
    PIMAGE_NT_HEADERS NtHeader;
    ULONG TimeStamp;
    BOOLEAN FirstRun = TRUE;

    /* Loop parameters */
    for (i = 0; i < ParameterCount; i++)
    {
        /* Get the base for this parameter */
        ImageBase = KiPcToFileHeader((PVOID)Parameters[i],
                                     &LdrEntry,
                                     FALSE,
                                     &InSystem);
        if (!ImageBase)
        {
            /* FIXME: Add code to check for unloaded drivers */
            DPRINT1("Potentially unloaded driver!\n");
            continue;
        }
        else
        {
            /* Get the NT Headers and Timestamp */
            NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
            TimeStamp = NtHeader->FileHeader.TimeDateStamp;

            /* Convert the driver name */
            DriverName = &LdrEntry->BaseDllName;
            ConversionRoutine(&LdrEntry->BaseDllName,
                              AnsiName,
                              sizeof(AnsiName));
        }

        /* Format driver name */
        sprintf(Message,
                "%s**  %12s - Address %p base at %p, DateStamp %08lx\r\n",
                FirstRun ? "\r\n*":"*",
                AnsiName,
                (PVOID)Parameters[i],
                ImageBase,
                TimeStamp);

        /* Check if we only had one parameter */
        if (ParameterCount <= 1)
        {
            /* Then just save the name */
            KiBugCheckDriver = DriverName;
        }
        else
        {
            /* Otherwise, display the message */
            InbvDisplayString(Message);
        }

        /* Loop again */
        FirstRun = FALSE;
    }
}

VOID
NTAPI
KiDisplayBlueScreen(IN ULONG MessageId,
                    IN BOOLEAN IsHardError,
                    IN PCHAR HardErrCaption OPTIONAL,
                    IN PCHAR HardErrMessage OPTIONAL,
                    IN PCHAR Message)
{
    ULONG BugCheckCode = (ULONG)KiBugCheckData[0];
    BOOLEAN Enable = TRUE;
    CHAR AnsiName[107];

    /* Enable headless support for bugcheck */
    HeadlessDispatch(HeadlessCmdStartBugCheck,
                     NULL, 0, NULL, NULL);
    HeadlessDispatch(HeadlessCmdEnableTerminal,
                     &Enable, sizeof(Enable),
                     NULL, NULL);
    HeadlessDispatch(HeadlessCmdSendBlueScreenData,
                     &BugCheckCode, sizeof(BugCheckCode),
                     NULL, NULL);

    /* Check if bootvid is installed */
    if (InbvIsBootDriverInstalled())
    {
        /* Acquire ownership and reset the display */
        InbvAcquireDisplayOwnership();
        InbvResetDisplay();

        /* Display blue screen */
        InbvSolidColorFill(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BV_COLOR_BLUE);
        InbvSetTextColor(BV_COLOR_WHITE);
        InbvInstallDisplayStringFilter(NULL);
        InbvEnableDisplayString(TRUE);
        InbvSetScrollRegion(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    }

    /* Check if this is a hard error */
    if (IsHardError)
    {
        /* Display caption and message */
        if (HardErrCaption) InbvDisplayString(HardErrCaption);
        if (HardErrMessage) InbvDisplayString(HardErrMessage);
    }

    /* Begin the display */
    InbvDisplayString("\r\n");

    /* Print out initial message */
    KeGetBugMessageText(BUGCHECK_MESSAGE_INTRO, NULL);
    InbvDisplayString("\r\n\r\n");

    /* Check if we have a driver */
    if (KiBugCheckDriver)
    {
        /* Print out into to driver name */
        KeGetBugMessageText(BUGCODE_ID_DRIVER, NULL);

        /* Convert and print out driver name */
        KeBugCheckUnicodeToAnsi(KiBugCheckDriver, AnsiName, sizeof(AnsiName));
        InbvDisplayString(" ");
        InbvDisplayString(AnsiName);
        InbvDisplayString("\r\n\r\n");
    }

    /* Check if this is the generic message */
    if (MessageId == BUGCODE_PSS_MESSAGE)
    {
        /* It is, so get the bug code string as well */
        KeGetBugMessageText(BugCheckCode, NULL);
        InbvDisplayString("\r\n\r\n");
    }

    /* Print second introduction message */
    KeGetBugMessageText(PSS_MESSAGE_INTRO, NULL);
    InbvDisplayString("\r\n\r\n");

    /* Get the bug code string */
    KeGetBugMessageText(MessageId, NULL);
    InbvDisplayString("\r\n\r\n");

    /* Print message for technical information */
    KeGetBugMessageText(BUGCHECK_TECH_INFO, NULL);

    /* Show the technical Data */
    RtlStringCbPrintfA(AnsiName,
                       sizeof(AnsiName),
                       "\r\n\r\n*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)\r\n\r\n",
                       BugCheckCode,
                       (PVOID)KiBugCheckData[1],
                       (PVOID)KiBugCheckData[2],
                       (PVOID)KiBugCheckData[3],
                       (PVOID)KiBugCheckData[4]);
    InbvDisplayString(AnsiName);

    /* Check if we have a driver*/
    if (KiBugCheckDriver)
    {
        /* Display technical driver data */
        InbvDisplayString(Message);
    }
    else
    {
        /* Dump parameter information */
        KiDumpParameterImages(Message,
                              (PVOID)&KiBugCheckData[1],
                              4,
                              KeBugCheckUnicodeToAnsi);
    }
}

DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckWithTf(IN ULONG BugCheckCode,
                 IN ULONG_PTR BugCheckParameter1,
                 IN ULONG_PTR BugCheckParameter2,
                 IN ULONG_PTR BugCheckParameter3,
                 IN ULONG_PTR BugCheckParameter4,
                 IN PKTRAP_FRAME TrapFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    CONTEXT Context;
    ULONG MessageId;
    CHAR AnsiName[128];
    BOOLEAN IsSystem, IsHardError = FALSE, Reboot = FALSE;
    PCHAR HardErrCaption = NULL, HardErrMessage = NULL;
    PVOID Pc = NULL, Memory;
    PVOID DriverBase;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PULONG_PTR HardErrorParameters;
    KIRQL OldIrql;

    /* Set active bugcheck */
    KeBugCheckActive = TRUE;
    KiBugCheckDriver = NULL;

    /* Check if this is power failure simulation */
    if (BugCheckCode == POWER_FAILURE_SIMULATE)
    {
        /* Call the Callbacks and reboot */
        KiDoBugCheckCallbacks();
        HalReturnToFirmware(HalRebootRoutine);
    }

    /* Save the IRQL and set hardware trigger */
    Prcb->DebuggerSavedIRQL = KeGetCurrentIrql();
    InterlockedIncrement((PLONG)&KiHardwareTrigger);

    /* Capture the CPU Context */
    RtlCaptureContext(&Prcb->ProcessorState.ContextFrame);
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    Context = Prcb->ProcessorState.ContextFrame;

    /* FIXME: Call the Watchdog if it's registered */

    /* Check which bugcode this is */
    switch (BugCheckCode)
    {
        /* These bug checks already have detailed messages, keep them */
        case UNEXPECTED_KERNEL_MODE_TRAP:
        case DRIVER_CORRUPTED_EXPOOL:
        case ACPI_BIOS_ERROR:
        case ACPI_BIOS_FATAL_ERROR:
        case THREAD_STUCK_IN_DEVICE_DRIVER:
        case DATA_BUS_ERROR:
        case FAT_FILE_SYSTEM:
        case NO_MORE_SYSTEM_PTES:
        case INACCESSIBLE_BOOT_DEVICE:

            /* Keep the same code */
            MessageId = BugCheckCode;
            break;

        /* Check if this is a kernel-mode exception */
        case KERNEL_MODE_EXCEPTION_NOT_HANDLED:
        case SYSTEM_THREAD_EXCEPTION_NOT_HANDLED:
        case KMODE_EXCEPTION_NOT_HANDLED:

            /* Use the generic text message */
            MessageId = KMODE_EXCEPTION_NOT_HANDLED;
            break;

        /* File-system errors */
        case NTFS_FILE_SYSTEM:

            /* Use the generic message for FAT */
            MessageId = FAT_FILE_SYSTEM;
            break;

        /* Check if this is a coruption of the Mm's Pool */
        case DRIVER_CORRUPTED_MMPOOL:

            /* Use generic corruption message */
            MessageId = DRIVER_CORRUPTED_EXPOOL;
            break;

        /* Check if this is a signature check failure */
        case STATUS_SYSTEM_IMAGE_BAD_SIGNATURE:

            /* Use the generic corruption message */
            MessageId = BUGCODE_PSS_MESSAGE_SIGNATURE;
            break;

        /* All other codes */
        default:

            /* Use the default bugcheck message */
            MessageId = BUGCODE_PSS_MESSAGE;
            break;
    }

    /* Save bugcheck data */
    KiBugCheckData[0] = BugCheckCode;
    KiBugCheckData[1] = BugCheckParameter1;
    KiBugCheckData[2] = BugCheckParameter2;
    KiBugCheckData[3] = BugCheckParameter3;
    KiBugCheckData[4] = BugCheckParameter4;

    /* Now check what bugcheck this is */
    switch (BugCheckCode)
    {
        /* Invalid access to R/O memory or Unhandled KM Exception */
        case KERNEL_MODE_EXCEPTION_NOT_HANDLED:
        case ATTEMPTED_WRITE_TO_READONLY_MEMORY:
        case ATTEMPTED_EXECUTE_OF_NOEXECUTE_MEMORY:
        {
            /* Check if we have a trap frame */
            if (!TrapFrame)
            {
                /* Use parameter 3 as a trap frame, if it exists */
                if (BugCheckParameter3) TrapFrame = (PVOID)BugCheckParameter3;
            }

            /* Check if we got one now and if we need to get the Program Counter */
            if ((TrapFrame) &&
                (BugCheckCode != KERNEL_MODE_EXCEPTION_NOT_HANDLED))
            {
                /* Get the Program Counter */
                Pc = (PVOID)KeGetTrapFramePc(TrapFrame);
            }
            break;
        }

        /* Wrong IRQL */
        case IRQL_NOT_LESS_OR_EQUAL:
        {
            /*
             * The NT kernel has 3 special sections:
             * MISYSPTE, POOLMI and POOLCODE. The bug check code can
             * determine in which of these sections this bugcode happened
             * and provide a more detailed analysis. For now, we don't.
             */

            /* Program Counter is in parameter 4 */
            Pc = (PVOID)BugCheckParameter4;

            /* Get the driver base */
            DriverBase = KiPcToFileHeader(Pc,
                                          &LdrEntry,
                                          FALSE,
                                          &IsSystem);
            if (IsSystem)
            {
                /*
                 * The error happened inside the kernel or HAL.
                 * Get the memory address that was being referenced.
                 */
                Memory = (PVOID)BugCheckParameter1;

                /* Find to which driver it belongs */
                DriverBase = KiPcToFileHeader(Memory,
                                              &LdrEntry,
                                              TRUE,
                                              &IsSystem);
                if (DriverBase)
                {
                    /* Get the driver name and update the bug code */
                    KiBugCheckDriver = &LdrEntry->BaseDllName;
                    KiBugCheckData[0] = DRIVER_PORTION_MUST_BE_NONPAGED;
                }
                else
                {
                    /* Find the driver that unloaded at this address */
                    KiBugCheckDriver = NULL; // FIXME: ROS can't locate

                    /* Check if the cause was an unloaded driver */
                    if (KiBugCheckDriver)
                    {
                        /* Update bug check code */
                        KiBugCheckData[0] =
                            SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD;
                    }
                }
            }
            else
            {
                /* Update the bug check code */
                KiBugCheckData[0] = DRIVER_IRQL_NOT_LESS_OR_EQUAL;
            }

            /* Clear Pc so we don't look it up later */
            Pc = NULL;
            break;
        }

        /* Hard error */
        case FATAL_UNHANDLED_HARD_ERROR:
        {
            /* Copy bug check data from hard error */
            HardErrorParameters = (PULONG_PTR)BugCheckParameter2;
            KiBugCheckData[0] = BugCheckParameter1;
            KiBugCheckData[1] = HardErrorParameters[0];
            KiBugCheckData[2] = HardErrorParameters[1];
            KiBugCheckData[3] = HardErrorParameters[2];
            KiBugCheckData[4] = HardErrorParameters[3];

            /* Remember that this is hard error and set the caption/message */
            IsHardError = TRUE;
            HardErrCaption = (PCHAR)BugCheckParameter3;
            HardErrMessage = (PCHAR)BugCheckParameter4;
            break;
        }

        /* Page fault */
        case PAGE_FAULT_IN_NONPAGED_AREA:
        {
            /* Assume no driver */
            DriverBase = NULL;

            /* Check if we have a trap frame */
            if (!TrapFrame)
            {
                /* We don't, use parameter 3 if possible */
                if (BugCheckParameter3) TrapFrame = (PVOID)BugCheckParameter3;
            }

            /* Check if we have a frame now */
            if (TrapFrame)
            {
                /* Get the Program Counter */
                Pc = (PVOID)KeGetTrapFramePc(TrapFrame);
                KiBugCheckData[3] = (ULONG_PTR)Pc;

                /* Find out if was in the kernel or drivers */
                DriverBase = KiPcToFileHeader(Pc,
                                              &LdrEntry,
                                              FALSE,
                                              &IsSystem);
            }
            else
            {
                /* Can't blame a driver, assume system */
                IsSystem = TRUE;
            }

            /* FIXME: Check for session pool in addition to special pool */

            /* Special pool has its own bug check codes */
            if (MmIsSpecialPoolAddress((PVOID)BugCheckParameter1))
            {
                if (MmIsSpecialPoolAddressFree((PVOID)BugCheckParameter1))
                {
                    KiBugCheckData[0] = IsSystem
                        ? PAGE_FAULT_IN_FREED_SPECIAL_POOL
                        : DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL;
                }
                else
                {
                    KiBugCheckData[0] = IsSystem
                        ? PAGE_FAULT_BEYOND_END_OF_ALLOCATION
                        : DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION;
                }
            }
            else if (!DriverBase)
            {
                /* Find the driver that unloaded at this address */
                KiBugCheckDriver = NULL; // FIXME: ROS can't locate

                /* Check if the cause was an unloaded driver */
                if (KiBugCheckDriver)
                {
                    KiBugCheckData[0] =
                        DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS;
                }
            }
            break;
        }

        /* Check if the driver forgot to unlock pages */
        case DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS:

            /* Program Counter is in parameter 1 */
            Pc = (PVOID)BugCheckParameter1;
            break;

        /* Check if the driver consumed too many PTEs */
        case DRIVER_USED_EXCESSIVE_PTES:

            /* Loader entry is in parameter 1 */
            LdrEntry = (PVOID)BugCheckParameter1;
            KiBugCheckDriver = &LdrEntry->BaseDllName;
            break;

        /* Check if the driver has a stuck thread */
        case THREAD_STUCK_IN_DEVICE_DRIVER:

            /* The name is in Parameter 3 */
            KiBugCheckDriver = (PVOID)BugCheckParameter3;
            break;

        /* Anything else */
        default:
            break;
    }

    /* Do we have a driver name? */
    if (KiBugCheckDriver)
    {
        /* Convert it to ANSI */
        KeBugCheckUnicodeToAnsi(KiBugCheckDriver, AnsiName, sizeof(AnsiName));
    }
    else
    {
        /* Do we have a Program Counter? */
        if (Pc)
        {
            /* Dump image name */
            KiDumpParameterImages(AnsiName,
                                  (PULONG_PTR)&Pc,
                                  1,
                                  KeBugCheckUnicodeToAnsi);
        }
    }

    /* Check if we need to save the context for KD */
    if (!KdPitchDebugger) KdDebuggerDataBlock.SavedContext = (ULONG_PTR)&Context;

    /* Check if a debugger is connected */
    if ((BugCheckCode != MANUALLY_INITIATED_CRASH) && (KdDebuggerEnabled))
    {
        /* Crash on the debugger console */
        DbgPrint("\n*** Fatal System Error: 0x%08lx\n"
                 "                       (0x%p,0x%p,0x%p,0x%p)\n\n",
                 KiBugCheckData[0],
                 KiBugCheckData[1],
                 KiBugCheckData[2],
                 KiBugCheckData[3],
                 KiBugCheckData[4]);

        /* Check if the debugger isn't currently connected */
        if (!KdDebuggerNotPresent)
        {
            /* Check if we have a driver to blame */
            if (KiBugCheckDriver)
            {
                /* Dump it */
                DbgPrint("Driver at fault: %s.\n", AnsiName);
            }

            /* Check if this was a hard error */
            if (IsHardError)
            {
                /* Print caption and message */
                if (HardErrCaption) DbgPrint(HardErrCaption);
                if (HardErrMessage) DbgPrint(HardErrMessage);
            }

            /* Break in the debugger */
            KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_FIRST);
        }
    }

    /* Raise IRQL to HIGH_LEVEL */
    _disable();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Avoid recursion */
    if (!InterlockedDecrement((PLONG)&KeBugCheckCount))
    {
#ifdef CONFIG_SMP
        /* Set CPU that is bug checking now */
        KeBugCheckOwner = Prcb->Number;

        /* Freeze the other CPUs */
        KxFreezeExecution();
#endif

        /* Display the BSOD */
        KiDisplayBlueScreen(MessageId,
                            IsHardError,
                            HardErrCaption,
                            HardErrMessage,
                            AnsiName);

        // TODO/FIXME: Run the registered reason-callbacks from
        // the KeBugcheckReasonCallbackListHead list with the
        // KbCallbackReserved1 reason.

        /* Check if the debugger is disabled but we can enable it */
        if (!(KdDebuggerEnabled) && !(KdPitchDebugger))
        {
            /* Enable it */
            KdEnableDebuggerWithLock(FALSE);
        }
        else
        {
            /* Otherwise, print the last line */
            InbvDisplayString("\r\n");
        }

        /* Save the context */
        Prcb->ProcessorState.ContextFrame = Context;

        /* FIXME: Support Triage Dump */

        /* FIXME: Write the crash dump */
        // TODO: The crash-dump helper must set the Reboot variable.
        Reboot = !!IopAutoReboot;
    }
    else
    {
        /* Increase recursion count */
        KeBugCheckOwnerRecursionCount++;
        if (KeBugCheckOwnerRecursionCount == 2)
        {
            /* Break in the debugger */
            KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_SECOND);
        }
        else if (KeBugCheckOwnerRecursionCount > 2)
        {
            /* Halt execution */
            while (TRUE);
        }
    }

    /* Call the Callbacks */
    KiDoBugCheckCallbacks();

    /* FIXME: Call Watchdog if enabled */

    /* Check if we have to reboot */
    if (Reboot)
    {
        /* Unload symbols */
        DbgUnLoadImageSymbols(NULL, (PVOID)MAXULONG_PTR, 0);
        HalReturnToFirmware(HalRebootRoutine);
    }

    /* Attempt to break in the debugger (otherwise halt CPU) */
    KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_SECOND);

    /* Shouldn't get here */
    ASSERT(FALSE);
    while (TRUE);
}

BOOLEAN
NTAPI
KiHandleNmi(VOID)
{
    BOOLEAN Handled = FALSE;
    PKNMI_HANDLER_CALLBACK NmiData;

    /* Parse the list of callbacks */
    NmiData = KiNmiCallbackListHead;
    while (NmiData)
    {
        /* Save if this callback has handled it -- all it takes is one */
        Handled |= NmiData->Callback(NmiData->Context, Handled);
        NmiData = NmiData->Next;
    }

    /* Has anyone handled this? */
    return Handled;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KeInitializeCrashDumpHeader(IN ULONG Type,
                            IN ULONG Flags,
                            OUT PVOID Buffer,
                            IN ULONG BufferSize,
                            OUT ULONG BufferNeeded OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeDeregisterBugCheckCallback(IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State */
    if (CallbackRecord->State == BufferInserted)
    {
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
NTAPI
KeDeregisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State */
    if (CallbackRecord->State == BufferInserted)
    {
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
NTAPI
KeRegisterBugCheckCallback(IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
                           IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
                           IN PVOID Buffer,
                           IN ULONG Length,
                           IN PUCHAR Component)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State first so we don't double-register */
    if (CallbackRecord->State == BufferEmpty)
    {
        /* Set the Callback Settings and insert into the list */
        CallbackRecord->Length = Length;
        CallbackRecord->Buffer = Buffer;
        CallbackRecord->Component = Component;
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->State = BufferInserted;
        InsertTailList(&KeBugcheckCallbackListHead, &CallbackRecord->Entry);
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
NTAPI
KeRegisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PUCHAR Component)
{
    KIRQL OldIrql;
    BOOLEAN Status = FALSE;

    /* Raise IRQL to High */
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    /* Check the Current State first so we don't double-register */
    if (CallbackRecord->State == BufferEmpty)
    {
        /* Set the Callback Settings and insert into the list */
        CallbackRecord->Component = Component;
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->State = BufferInserted;
        CallbackRecord->Reason = Reason;
        InsertTailList(&KeBugcheckReasonCallbackListHead,
                       &CallbackRecord->Entry);
        Status = TRUE;
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
}

/*
 * @implemented
 */
PVOID
NTAPI
KeRegisterNmiCallback(IN PNMI_CALLBACK CallbackRoutine,
                      IN PVOID Context)
{
    KIRQL OldIrql;
    PKNMI_HANDLER_CALLBACK NmiData, Next;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Allocate NMI callback data */
    NmiData = ExAllocatePoolWithTag(NonPagedPool, sizeof(*NmiData), TAG_KNMI);
    if (!NmiData) return NULL;

    /* Fill in the information */
    NmiData->Callback = CallbackRoutine;
    NmiData->Context = Context;
    NmiData->Handle = NmiData;

    /* Insert it into NMI callback list */
    KiAcquireNmiListLock(&OldIrql);
    NmiData->Next = KiNmiCallbackListHead;
    Next = InterlockedCompareExchangePointer((PVOID*)&KiNmiCallbackListHead,
                                             NmiData,
                                             NmiData->Next);
    ASSERT(Next == NmiData->Next);
    KiReleaseNmiListLock(OldIrql);

    /* Return the opaque "handle" */
    return NmiData->Handle;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeDeregisterNmiCallback(IN PVOID Handle)
{
    KIRQL OldIrql;
    PKNMI_HANDLER_CALLBACK NmiData;
    PKNMI_HANDLER_CALLBACK* Previous;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Find in the list the NMI callback corresponding to the handle */
    KiAcquireNmiListLock(&OldIrql);
    Previous = &KiNmiCallbackListHead;
    NmiData = *Previous;
    while (NmiData)
    {
        if (NmiData->Handle == Handle)
        {
            /* The handle is the pointer to the callback itself */
            ASSERT(Handle == NmiData);

            /* Found it, remove from the list */
            *Previous = NmiData->Next;
            break;
        }

        /* Not found; try again */
        Previous = &NmiData->Next;
        NmiData = *Previous;
    }
    KiReleaseNmiListLock(OldIrql);

    /* If we have found the entry, free it */
    if (NmiData)
    {
        ExFreePoolWithTag(NmiData, TAG_KNMI);
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_HANDLE;
}

/*
 * @implemented
 */
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckEx(IN ULONG BugCheckCode,
             IN ULONG_PTR BugCheckParameter1,
             IN ULONG_PTR BugCheckParameter2,
             IN ULONG_PTR BugCheckParameter3,
             IN ULONG_PTR BugCheckParameter4)
{
    /* Call the internal API */
    KeBugCheckWithTf(BugCheckCode,
                     BugCheckParameter1,
                     BugCheckParameter2,
                     BugCheckParameter3,
                     BugCheckParameter4,
                     NULL);
}

/*
 * @implemented
 */
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck(ULONG BugCheckCode)
{
    /* Call the internal API */
    KeBugCheckWithTf(BugCheckCode, 0, 0, 0, 0, NULL);
}

/*
 * @implemented
 */
VOID
NTAPI
KeEnterKernelDebugger(VOID)
{
    /* Disable interrupts */
    KiHardwareTrigger = 1;
    _disable();

    /* Check the bugcheck count */
    if (!InterlockedDecrement((PLONG)&KeBugCheckCount))
    {
        /* There was only one, is the debugger disabled? */
        if (!(KdDebuggerEnabled) && !(KdPitchDebugger))
        {
            /* Enable the debugger */
            KdInitSystem(0, NULL);
        }
    }

    /* Break in the debugger */
    KiBugCheckDebugBreak(DBG_STATUS_FATAL);
}

/* EOF */
