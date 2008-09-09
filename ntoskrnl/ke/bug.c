/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Bugcheck Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, KiInitializeBugCheck)
#endif

/* GLOBALS *******************************************************************/

LIST_ENTRY KeBugcheckCallbackListHead;
LIST_ENTRY KeBugcheckReasonCallbackListHead;
KSPIN_LOCK BugCheckCallbackLock;
ULONG KeBugCheckActive, KeBugCheckOwner;
LONG KeBugCheckOwnerRecursionCount;
PRTL_MESSAGE_RESOURCE_DATA KiBugCodeMessages;
ULONG KeBugCheckCount = 1;
ULONG KiHardwareTrigger;
PUNICODE_STRING KiBugCheckDriver;
ULONG_PTR KiBugCheckData[5];

/* Bugzilla Reporting */
UNICODE_STRING KeRosProcessorName, KeRosBiosDate, KeRosBiosVersion;
UNICODE_STRING KeRosVideoBiosDate, KeRosVideoBiosVersion;

/* PRIVATE FUNCTIONS *********************************************************/

PVOID
NTAPI
KiPcToFileHeader(IN PVOID Eip,
                 OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                 IN BOOLEAN DriversOnly,
                 OUT PBOOLEAN InKernel)
{
    ULONG i = 0;
    PVOID ImageBase, EipBase = NULL;
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
            if ((i <= 2) && (DriversOnly == TRUE))
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
            if (((ULONG_PTR)Eip >= (ULONG_PTR)Entry->DllBase) &&
                ((ULONG_PTR)Eip < ((ULONG_PTR)Entry->DllBase + Entry->SizeOfImage)))
            {
                /* Return this entry */
                *LdrEntry = Entry;
                EipBase = ImageBase;

                /* Check if this was a kernel or HAL entry */
                if (i <= 2) *InKernel = TRUE;
                break;
            }
        }
    }

    /* Return the base address */
    return EipBase;
}

BOOLEAN
NTAPI
KiRosPrintAddress(PVOID address)
{
    PLIST_ENTRY current_entry;
    PLDR_DATA_TABLE_ENTRY current;
    extern LIST_ENTRY PsLoadedModuleList;
    ULONG_PTR RelativeAddress;
    ULONG i = 0;

    do
    {
        current_entry = PsLoadedModuleList.Flink;

        while (current_entry != &PsLoadedModuleList)
        {
            current = CONTAINING_RECORD(current_entry,
                                        LDR_DATA_TABLE_ENTRY,
                                        InLoadOrderLinks);

            if (address >= (PVOID)current->DllBase &&
                address < (PVOID)((ULONG_PTR)current->DllBase +
                                             current->SizeOfImage))
            {
                RelativeAddress = (ULONG_PTR)address -
                                  (ULONG_PTR)current->DllBase;
                DbgPrint("<%wZ: %x>", &current->FullDllName, RelativeAddress);
                return(TRUE);
            }
            current_entry = current_entry->Flink;
        }
    } while(++i <= 1);

    return(FALSE);
}

PVOID
NTAPI
KiRosPcToUserFileHeader(IN PVOID Eip,
                        OUT PLDR_DATA_TABLE_ENTRY *LdrEntry)
{
    PVOID ImageBase, EipBase = NULL;
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
            if (((ULONG_PTR)Eip >= (ULONG_PTR)Entry->DllBase) &&
                ((ULONG_PTR)Eip < ((ULONG_PTR)Entry->DllBase + Entry->SizeOfImage)))
            {
                /* Return this entry */
                *LdrEntry = Entry;
                EipBase = ImageBase;
                break;
            }
        }
    }

    /* Return the base address */
    return EipBase;
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
KeRosDumpStackFrameArray(IN PULONG Frames,
                         IN ULONG FrameCount)
{
    ULONG i, Addr;
    BOOLEAN InSystem;
    PVOID p;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

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
            if (!KdbSymPrintAddress((PVOID)Addr))
#endif
            {
                /* Print out the module name */
                Addr -= (ULONG_PTR)LdrEntry->DllBase;
                DbgPrint("<%wZ: %x>\n", &LdrEntry->FullDllName, Addr);
            }
        }
        else
        {
            /* Print only the address */
            DbgPrint("<%x>\n", Addr);
        }

        /* Go to the next frame */
        DbgPrint("\n");
    }
}

VOID
NTAPI
KeRosDumpStackFrames(IN PULONG Frame OPTIONAL,
                     IN ULONG FrameCount OPTIONAL)
{
    ULONG Frames[32];
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

VOID
NTAPI
KeRosDumpTriageForBugZillaReport(VOID)
{
#if 0
    extern BOOLEAN KiFastSystemCallDisable, KiSMTProcessorsPresent;
    extern ULONG KeI386MachineType, MxcsrFeatureMask;
    extern BOOLEAN Ke386Pae, Ke386NoExecute;

    DbgPrint("ReactOS has crashed! Please go to http://www.reactos.org/bugzilla/enter_bug.cgi to file a bug!\n");
    DbgPrint("\nHardware Information\n");
    DbgPrint("Processor Architecture: %d\n"
             "Feature Bits: %d\n"
             "System Call Disabled: %d\n"
             "NPX Present: %d\n"
             "MXCsr Mask: %d\n"
             "MXCsr Feature Mask: %d\n"
             "XMMI Present: %d\n"
             "FXSR Present: %d\n"
             "Machine Type: %d\n"
             "PAE: %d\n"
             "NX: %d\n"
             "Processors: %d\n"
             "Active Processors: %d\n"
             "Pentium LOCK Bug: %d\n"
             "Hyperthreading: %d\n"
             "CPU Manufacturer: %s\n"
             "CPU Name: %wZ\n"
             "CPUID: %d\n"
             "CPU Type: %d\n"
             "CPU Stepping: %d\n"
             "CPU Speed: %d\n"
             "CPU L2 Cache: %d\n"
             "BIOS Date: %wZ\n"
             "BIOS Version: %wZ\n"
             "Video BIOS Date: %wZ\n"
             "Video BIOS Version: %wZ\n"
             "Memory: %d\n",
             KeProcessorArchitecture,
             KeFeatureBits,
             KiFastSystemCallDisable,
             KeI386NpxPresent,
             KiMXCsrMask,
             MxcsrFeatureMask,
             KeI386XMMIPresent,
             KeI386FxsrPresent,
             KeI386MachineType,
             Ke386Pae,
             Ke386NoExecute,
             KeNumberProcessors,
             KeActiveProcessors,
             KiI386PentiumLockErrataPresent,
             KiSMTProcessorsPresent,
             KeGetCurrentPrcb()->VendorString,
             &KeRosProcessorName,
             KeGetCurrentPrcb()->CpuID,
             KeGetCurrentPrcb()->CpuType,
             KeGetCurrentPrcb()->CpuStep,
             KeGetCurrentPrcb()->MHz,
             ((PKIPCR)KeGetPcr())->SecondLevelCacheSize,
             &KeRosBiosDate,
             &KeRosBiosVersion,
             &KeRosVideoBiosDate,
             &KeRosVideoBiosVersion,
             MmNumberOfPhysicalPages * PAGE_SIZE);
#endif
}

VOID
INIT_FUNCTION
NTAPI
KiInitializeBugCheck(VOID)
{
    PRTL_MESSAGE_RESOURCE_DATA BugCheckData;
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
    ULONG_PTR MessageEntry;
    PCHAR BugCode;
    BOOLEAN Result = FALSE;

    /* Make sure we're not bugchecking too early */
    if (!KiBugCodeMessages) return Result;

    /* Find the message. This code is based on RtlFindMesssage */
    for (i = 0; i < KiBugCodeMessages->NumberOfBlocks; i++)
    {
        /* Check if the ID Matches */
        if ((BugCheckCode >= KiBugCodeMessages->Blocks[i].LowId) &&
            (BugCheckCode <= KiBugCodeMessages->Blocks[i].HighId))
        {
            /* Get Offset to Entry */
            MessageEntry = KiBugCodeMessages->Blocks[i].OffsetToEntries +
                           (ULONG_PTR)KiBugCodeMessages;
            IdOffset = BugCheckCode - KiBugCodeMessages->Blocks[i].LowId;

            /* Get offset to ID */
            for (i = 0; i < IdOffset; i++)
            {
                /* Advance in the Entries */
                MessageEntry += ((PRTL_MESSAGE_RESOURCE_ENTRY)MessageEntry)->
                                Length;
            }

            /* Get the final Code */
            BugCode = ((PRTL_MESSAGE_RESOURCE_ENTRY)MessageEntry)->Text;
            i = strlen(BugCode);

            /* Handle newlines */
            while ((i > 0) && ((BugCode[i] == '\n') ||
                               (BugCode[i] == '\r') ||
                               (BugCode[i] == ANSI_NULL)))
            {
                /* Check if we have a string to return */
                if (!OutputString) BugCode[i] = ANSI_NULL;
                i--;
            }

            /* Check if caller wants an output string */
            if (OutputString)
            {
                /* Return it in the OutputString */
                OutputString->Buffer = BugCode;
                OutputString->Length = (USHORT)i + 1;
                OutputString->MaximumLength = (USHORT)i + 1;
            }
            else
            {
                /* Direct Output to Screen */
                InbvDisplayString(BugCode);
                InbvDisplayString("\r");
            }

            /* We're done */
            Result = TRUE;
            break;
        }
    }

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

    /* First make sure that the list is Initialized... it might not be */
    ListHead = &KeBugcheckCallbackListHead;
    if ((ListHead->Flink) && (ListHead->Blink))
    {
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
            if (CurrentRecord->Entry.Blink != LastEntry) return;
            Checksum = (ULONG_PTR)CurrentRecord->CallbackRoutine;
            Checksum += (ULONG_PTR)CurrentRecord->Buffer;
            Checksum += (ULONG_PTR)CurrentRecord->Length;
            Checksum += (ULONG_PTR)CurrentRecord->Component;

            /* Make sure it's inserted and valitdated */
            if ((CurrentRecord->State == BufferInserted) &&
                (CurrentRecord->Checksum == Checksum))
            {
                /* Call the routine */
                CurrentRecord->State = BufferStarted;
                (CurrentRecord->CallbackRoutine)(CurrentRecord->Buffer,
                                                 CurrentRecord->Length);
                CurrentRecord->State = BufferFinished;
            }

            /* Go to the next entry */
            LastEntry = NextEntry;
            NextEntry = NextEntry->Flink;
        }
    }
}

VOID
NTAPI
KiBugCheckDebugBreak(IN ULONG StatusCode)
{
    /* If KDBG isn't connected, freeze the CPU, otherwise, break */
    if (KdDebuggerNotPresent) for (;;) KeArchHaltProcessor();
    DbgBreakPointWithStatus(StatusCode);
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
            /* Driver wasn't found, check for unloaded driver */
            DriverName = NULL; // FIXME: ROS can't
            if (!DriverName) continue;

            /* Convert the driver name */
            ImageBase = (PVOID)Parameters[i];
            ConversionRoutine(DriverName, AnsiName, sizeof(AnsiName));
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
                "%s**  %12s - Address %p base at %p, DateStamp %08lx\n",
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
    CHAR AnsiName[75];

    /* Check if bootvid is installed */
    if (InbvIsBootDriverInstalled())
    {
        /* Acquire ownership and reset the display */
        InbvAcquireDisplayOwnership();
        InbvResetDisplay();

        /* Display blue screen */
        InbvSolidColorFill(0, 0, 639, 479, 4);
        InbvSetTextColor(15);
        InbvInstallDisplayStringFilter(NULL);
        InbvEnableDisplayString(TRUE);
        InbvSetScrollRegion(0, 0, 639, 479);
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
        KeGetBugMessageText(KiBugCheckData[0], NULL);
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
    sprintf(AnsiName,
            "\r\n\r\n*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)\r\n\r\n",
            KiBugCheckData[0],
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
    PVOID Eip = NULL, Memory;
    PVOID DriverBase;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PULONG_PTR HardErrorParameters;
    KIRQL OldIrql;
#ifdef CONFIG_SMP
    LONG i = 0;
#endif

    /* Set active bugcheck */
    KeBugCheckActive = TRUE;
    KiBugCheckDriver = NULL;

    /* Check if this is power failure simulation */
    if (BugCheckCode == POWER_FAILURE_SIMULATE)
    {
        /* Call the Callbacks and reboot */;
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
        //case SYSTEM_THREAD_EXCEPTION_NOT_HANDLED:
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

            /* Check if we have a trap frame */
            if (!TrapFrame)
            {
                /* Use parameter 3 as a trap frame, if it exists */
                if (BugCheckParameter3) TrapFrame = (PVOID)BugCheckParameter3;
            }

            /* Check if we got one now and if we need to get EIP */
            if ((TrapFrame) &&
                (BugCheckCode != KERNEL_MODE_EXCEPTION_NOT_HANDLED))
            {
#ifdef _M_IX86
                /* Get EIP */
                Eip = (PVOID)TrapFrame->Eip;
#elif defined(_M_PPC)
                Eip = (PVOID)TrapFrame->Dr0; /* srr0 */
#endif
            }
            break;

        /* Wrong IRQL */
        case IRQL_NOT_LESS_OR_EQUAL:

            /*
             * The NT kernel has 3 special sections:
             * MISYSPTE, POOLMI and POOLCODE. The bug check code can
             * determine in which of these sections this bugcode happened
             * and provide a more detailed analysis. For now, we don't.
             */

            /* Eip is in parameter 4 */
            Eip = (PVOID)BugCheckParameter4;

            /* Get the driver base */
            DriverBase = KiPcToFileHeader(Eip, &LdrEntry, FALSE, &IsSystem);
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

            /* Clear EIP so we don't look it up later */
            Eip = NULL;
            break;

        /* Hard error */
        case FATAL_UNHANDLED_HARD_ERROR:

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

        /* Page fault */
        case PAGE_FAULT_IN_NONPAGED_AREA:

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
#ifdef _M_IX86
                /* Get EIP */
                Eip = (PVOID)TrapFrame->Eip;
                KiBugCheckData[3] = (ULONG)Eip;
#elif defined(_M_PPC)
                Eip = (PVOID)TrapFrame->Dr0; /* srr0 */
                KiBugCheckData[3] = (ULONG)Eip;
#endif

                /* Find out if was in the kernel or drivers */
                DriverBase = KiPcToFileHeader(Eip,
                                              &LdrEntry,
                                              FALSE,
                                              &IsSystem);
            }

            /*
             * Now we should check if this happened in:
             * 1) Special Pool 2) Free Special Pool 3) Session Pool
             * and update the bugcheck code appropriately.
             */

            /* Check if we didn't have a driver base */
            if (!DriverBase)
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

        /* Check if the driver forgot to unlock pages */
        case DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS:

            /* EIP is in parameter 1 */
            Eip = (PVOID)BugCheckParameter1;
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
        /* Do we have an EIP? */
        if (Eip)
        {
            /* Dump image name */
            KiDumpParameterImages(AnsiName,
                                  (PULONG_PTR)&Eip,
                                  1,
                                  KeBugCheckUnicodeToAnsi);
        }
    }

    /* Check if we need to save the context for KD */
#ifdef _WINKD_
    if (!KdPitchDebugger) KdDebuggerDataBlock.SavedContext = (ULONG)&Context;
#endif

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
        else
        {
            /*
             * ROS HACK.
             * Ok, so debugging is enabled, but KDBG isn't there.
             * We'll manually dump the stack for the user.
             */
            KeRosDumpStackFrames(NULL, 0);

            /* ROS HACK 2: Generate something useful for Bugzilla */
            KeRosDumpTriageForBugZillaReport();
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
        for (i = 0; i < KeNumberProcessors; i++)
        {
            if (i != (LONG)KeGetCurrentProcessorNumber())
            {
                /* Send the IPI and give them one second to catch up */
                KiIpiSendRequest(1 << i, IPI_FREEZE);
                KeStallExecutionProcessor(1000000);
            }
        }
#endif

        /* Display the BSOD */
        KeLowerIrql(APC_LEVEL); // This is a nastier hack than any ever before
        KiDisplayBlueScreen(MessageId,
                            IsHardError,
                            HardErrCaption,
                            HardErrMessage,
                            AnsiName);
        KeRaiseIrql(HIGH_LEVEL, &OldIrql);

        /* Check if the debugger is disabled but we can enable it */
        if (!(KdDebuggerEnabled) && !(KdPitchDebugger))
        {
            /* Enable it */
#ifdef _WINKD_
            KdEnableDebuggerWithLock(FALSE);
#endif
        }
        else
        {
            /* Otherwise, print the last line */
            InbvDisplayString("\r\n");
        }

        /* Save the context */
        Prcb->ProcessorState.ContextFrame = Context;

        /* FIXME: Support Triage Dump */

        /* Write the crash dump */
        MmDumpToPagingFile(KiBugCheckData[4],
                           KiBugCheckData[0],
                           KiBugCheckData[1],
                           KiBugCheckData[2],
                           KiBugCheckData[3],
                           TrapFrame);
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
            /* Halt the CPU */
            for (;;) KeArchHaltProcessor();
        }
    }

    /* Call the Callbacks */
    KiDoBugCheckCallbacks();

    /* FIXME: Call Watchdog if enabled */

    /* Check if we have to reboot */
    if (Reboot)
    {
        /* Unload symbols */
        DbgUnLoadImageSymbols(NULL, NtCurrentProcess(), 0);
        HalReturnToFirmware(HalRebootRoutine);
    }

    /* Attempt to break in the debugger (otherwise halt CPU) */
    KiBugCheckDebugBreak(DBG_STATUS_BUGCHECK_SECOND);
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
 * @unimplemented
 */
NTSTATUS
NTAPI
KeDeregisterNmiCallback(PVOID Handle)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
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
 * @unimplemented
 */
PVOID
NTAPI
KeRegisterNmiCallback(IN PNMI_CALLBACK CallbackRoutine,
                      IN PVOID Context)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @implemented
 */
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

    /* Bugcheck */
    KiBugCheckDebugBreak(DBG_STATUS_FATAL);
}

/* EOF */
