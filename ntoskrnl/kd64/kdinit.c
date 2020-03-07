/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdinit.c
 * PURPOSE:         KD64 Initialization Code
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <reactos/buildno.h>
#define NDEBUG
#include <debug.h>

/* UTILITY FUNCTIONS *********************************************************/

/*
 * Get the total size of the memory before
 * Mm is initialized, by counting the number
 * of physical pages. Useful for debug logging.
 *
 * Strongly inspired by:
 * mm\ARM3\mminit.c : MiScanMemoryDescriptors(...)
 *
 * See also: kd\kdio.c
 */
static CODE_SEG("INIT")
SIZE_T
KdpGetMemorySizeInMBs(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY ListEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    SIZE_T NumberOfPhysicalPages = 0;

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        /* Check if this is invisible memory */
        if ((Descriptor->MemoryType == LoaderFirmwarePermanent) ||
            (Descriptor->MemoryType == LoaderSpecialMemory) ||
            (Descriptor->MemoryType == LoaderHALCachedMemory) ||
            (Descriptor->MemoryType == LoaderBBTMemory))
        {
            /* Skip this descriptor */
            continue;
        }

        /* Check if this is bad memory */
        if (Descriptor->MemoryType != LoaderBad)
        {
            /* Count this in the total of pages */
            NumberOfPhysicalPages += Descriptor->PageCount;
        }
    }

    /* Round size up. Assumed to better match actual physical RAM size */
    return ALIGN_UP_BY(NumberOfPhysicalPages * PAGE_SIZE, 1024 * 1024) / (1024 * 1024);
}

/* See also: kd\kdio.c */
static CODE_SEG("INIT")
VOID
KdpPrintBanner(IN SIZE_T MemSizeMBs)
{
    DPRINT1("-----------------------------------------------------\n");
    DPRINT1("ReactOS " KERNEL_VERSION_STR " (Build " KERNEL_VERSION_BUILD_STR ") (Commit " KERNEL_VERSION_COMMIT_HASH ")\n");
    DPRINT1("%u System Processor [%u MB Memory]\n", KeNumberProcessors, MemSizeMBs);

    if (KeLoaderBlock)
    {
        DPRINT1("Command Line: %s\n", KeLoaderBlock->LoadOptions);
        DPRINT1("ARC Paths: %s %s %s %s\n", KeLoaderBlock->ArcBootDeviceName, KeLoaderBlock->NtHalPathName, KeLoaderBlock->ArcHalDeviceName, KeLoaderBlock->NtBootPathName);
    }
}

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KdUpdateDataBlock(VOID)
{
#ifdef _WINKD_
    /* Update the KeUserCallbackDispatcher pointer */
    KdDebuggerDataBlock.KeUserCallbackDispatcher =
        (ULONG_PTR)KeUserCallbackDispatcher;
#endif
}

BOOLEAN
NTAPI
KdRegisterDebuggerDataBlock(IN ULONG Tag,
                            IN PDBGKD_DEBUG_DATA_HEADER64 DataHeader,
                            IN ULONG Size)
{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PDBGKD_DEBUG_DATA_HEADER64 CurrentHeader;

    /* Acquire the Data Lock */
    KeAcquireSpinLock(&KdpDataSpinLock, &OldIrql);

    /* Loop the debugger data list */
    NextEntry = KdpDebuggerDataListHead.Flink;
    while (NextEntry != &KdpDebuggerDataListHead)
    {
        /* Get the header for this entry */
        CurrentHeader = CONTAINING_RECORD(NextEntry,
                                          DBGKD_DEBUG_DATA_HEADER64,
                                          List);

        /*  Move to the next one */
        NextEntry = NextEntry->Flink;

        /* Check if we already have this data block */
        if ((CurrentHeader == DataHeader) || (CurrentHeader->OwnerTag == Tag))
        {
            /* Release the lock and fail */
            KeReleaseSpinLock(&KdpDataSpinLock, OldIrql);
            return FALSE;
        }
    }

    /* Setup the header */
    DataHeader->OwnerTag = Tag;
    DataHeader->Size = Size;

    /* Insert it into the list and release the lock */
    InsertTailList(&KdpDebuggerDataListHead, (PLIST_ENTRY)&DataHeader->List);
    KeReleaseSpinLock(&KdpDataSpinLock, OldIrql);
    return TRUE;
}

BOOLEAN
NTAPI
KdInitSystem(IN ULONG BootPhase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    BOOLEAN EnableKd, DisableKdAfterInit = FALSE, BlockEnable;
    LPSTR CommandLine, DebugLine, DebugOptionStart, DebugOptionEnd;
    STRING ImageName;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY NextEntry;
    ULONG i, j, Length;
    SIZE_T DebugOptionLength;
    SIZE_T MemSizeMBs;
    CHAR NameBuffer[256];
    PWCHAR Name;

#if defined(__GNUC__)
    /* Make gcc happy */
    BlockEnable = FALSE;
#endif

    /* Check if this is Phase 1 */
    if (BootPhase)
    {
        /* Just query the performance counter */
        KeQueryPerformanceCounter(&KdPerformanceCounterRate);
        return TRUE;
    }

    /* Check if we already initialized once */
    if (KdDebuggerEnabled) return TRUE;

    /* Set the Debug Routine as the Stub for now */
    KiDebugRoutine = KdpStub;

    /* Disable break after symbol load for now */
    KdBreakAfterSymbolLoad = FALSE;

    /* Check if the Debugger Data Block was already initialized */
    if (!KdpDebuggerDataListHead.Flink)
    {
        /* It wasn't...Initialize the KD Data Listhead */
        InitializeListHead(&KdpDebuggerDataListHead);

        /* Register the Debugger Data Block */
        KdRegisterDebuggerDataBlock(KDBG_TAG,
                                    &KdDebuggerDataBlock.Header,
                                    sizeof(KdDebuggerDataBlock));

        /* Fill out the KD Version Block */
        KdVersionBlock.MajorVersion = (USHORT)((DBGKD_MAJOR_NT << 8) | (NtBuildNumber >> 28));
        KdVersionBlock.MinorVersion = (USHORT)(NtBuildNumber & 0xFFFF);

#ifdef CONFIG_SMP
        /* This is an MP Build */
        KdVersionBlock.Flags |= DBGKD_VERS_FLAG_MP;
#endif

        /* Save Pointers to Loaded Module List and Debugger Data */
        KdVersionBlock.PsLoadedModuleList = (ULONG64)(LONG_PTR)&PsLoadedModuleList;
        KdVersionBlock.DebuggerDataList = (ULONG64)(LONG_PTR)&KdpDebuggerDataListHead;

        /* Set protocol limits */
        KdVersionBlock.MaxStateChange = DbgKdMaximumStateChange -
                                        DbgKdMinimumStateChange;
        KdVersionBlock.MaxManipulate = DbgKdMaximumManipulate -
                                       DbgKdMinimumManipulate;
        KdVersionBlock.Unused[0] = 0;

        /* Link us in the KPCR */
        KeGetPcr()->KdVersionBlock =  &KdVersionBlock;
    }

    /* Check if we have a loader block */
    if (LoaderBlock)
    {
        /* Get the image entry */
        LdrEntry = CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Save the Kernel Base */
        PsNtosImageBase = (ULONG_PTR)LdrEntry->DllBase;
        KdVersionBlock.KernBase = (ULONG64)(LONG_PTR)LdrEntry->DllBase;

        /* Check if we have a command line */
        CommandLine = LoaderBlock->LoadOptions;
        if (CommandLine)
        {
            /* Upcase it */
            _strupr(CommandLine);

            /* Assume we'll disable KD */
            EnableKd = FALSE;

            /* Check for CRASHDEBUG, NODEBUG and just DEBUG */
            if (strstr(CommandLine, "CRASHDEBUG"))
            {
                /* Don't enable KD now, but allow it to be enabled later */
                KdPitchDebugger = FALSE;
            }
            else if (strstr(CommandLine, "NODEBUG"))
            {
                /* Don't enable KD and don't let it be enabled later */
                KdPitchDebugger = TRUE;
            }
            else if ((DebugLine = strstr(CommandLine, "DEBUG")) != NULL)
            {
                /* Enable KD */
                EnableKd = TRUE;

#ifdef _WINKD_
                /* Check if there are any options */
                if (DebugLine[5] == '=')
                {
                    /* Save pointers */
                    DebugOptionStart = DebugOptionEnd = &DebugLine[6];

                    /* Scan the string for debug options */
                    for (;;)
                    {
                        /* Loop until we reach the end of the string */
                        while (*DebugOptionEnd != ANSI_NULL)
                        {
                            /* Check if this is a comma, a space or a tab */
                            if ((*DebugOptionEnd == ',') ||
                                (*DebugOptionEnd == ' ') ||
                                (*DebugOptionEnd == '\t'))
                            {
                                /*
                                 * We reached the end of the option or
                                 * the end of the string, break out
                                 */
                                break;
                            }
                            else
                            {
                                /* Move on to the next character */
                                DebugOptionEnd++;
                            }
                        }

                        /* Calculate the length of the current option */
                        DebugOptionLength = (DebugOptionEnd - DebugOptionStart);

                       /*
                        * Break out if we reached the last option
                        * or if there were no options at all
                        */
                       if (!DebugOptionLength) break;

                        /* Now check which option this is */
                        if ((DebugOptionLength == 10) &&
                            !(strncmp(DebugOptionStart, "AUTOENABLE", 10)))
                        {
                            /*
                             * Disable the debugger, but
                             * allow it to be reenabled
                             */
                            DisableKdAfterInit = TRUE;
                            BlockEnable = FALSE;
                            KdAutoEnableOnEvent = TRUE;
                        }
                        else if ((DebugOptionLength == 7) &&
                                 !(strncmp(DebugOptionStart, "DISABLE", 7)))
                        {
                            /* Disable the debugger */
                            DisableKdAfterInit = TRUE;
                            BlockEnable = TRUE;
                            KdAutoEnableOnEvent = FALSE;
                        }
                        else if ((DebugOptionLength == 6) &&
                                 !(strncmp(DebugOptionStart, "NOUMEX", 6)))
                        {
                            /* Ignore user mode exceptions */
                            KdIgnoreUmExceptions = TRUE;
                        }

                        /*
                         * If there are more options then
                         * the next character should be a comma
                         */
                        if (*DebugOptionEnd != ',')
                        {
                            /* It isn't, break out  */
                            break;
                        }

                        /* Move on to the next option */
                        DebugOptionEnd++;
                        DebugOptionStart = DebugOptionEnd;
                    }
                }
#else
		(VOID)DebugOptionStart;
		(VOID)DebugOptionEnd;
		(VOID)DebugOptionLength;
                KdDebuggerNotPresent = FALSE;
#ifdef KDBG
                /* Get the KDBG Settings */
                KdbpGetCommandLineSettings(LoaderBlock->LoadOptions);
#endif
#endif
            }
        }
        else
        {
            /* No command line options? Disable debugger by default */
            KdPitchDebugger = TRUE;
            EnableKd = FALSE;
        }
    }
    else
    {
        /* Called from a bugcheck or a re-enable. Save the Kernel Base. */
        KdVersionBlock.KernBase = (ULONG64)(LONG_PTR)PsNtosImageBase;

        /* Unconditionally enable KD */
        EnableKd = TRUE;
    }

    /* Set the Kernel Base in the Data Block */
    KdDebuggerDataBlock.KernBase = (ULONG_PTR)KdVersionBlock.KernBase;

    /* Initialize the debugger if requested */
    if (EnableKd && (NT_SUCCESS(KdDebuggerInitialize0(LoaderBlock))))
    {
        /* Now set our real KD routine */
        KiDebugRoutine = KdpTrap;

        /* Check if we've already initialized our structures */
        if (!KdpDebuggerStructuresInitialized)
        {
            /* Set the Debug Switch Routine and Retries */
            KdpContext.KdpDefaultRetries = 20;
            KiDebugSwitchRoutine = KdpSwitchProcessor;

            /* Initialize breakpoints owed flag and table */
            KdpOweBreakpoint = FALSE;
            for (i = 0; i < KD_BREAKPOINT_MAX; i++)
            {
                KdpBreakpointTable[i].Flags   = 0;
                KdpBreakpointTable[i].DirectoryTableBase = 0;
                KdpBreakpointTable[i].Address = NULL;
            }

            /* Initialize the Time Slip DPC */
            KeInitializeDpc(&KdpTimeSlipDpc, KdpTimeSlipDpcRoutine, NULL);
            KeInitializeTimer(&KdpTimeSlipTimer);
            ExInitializeWorkItem(&KdpTimeSlipWorkItem, KdpTimeSlipWork, NULL);

            /* First-time initialization done! */
            KdpDebuggerStructuresInitialized = TRUE;
        }

        /* Initialize the timer */
        KdTimerStart.QuadPart = 0;

        /* Officially enable KD */
        KdPitchDebugger = FALSE;
        KdDebuggerEnabled = TRUE;

        /* Let user-mode know that it's enabled as well */
        SharedUserData->KdDebuggerEnabled = TRUE;

        /* Display separator + ReactOS version at start of the debug log */
        MemSizeMBs = KdpGetMemorySizeInMBs(KeLoaderBlock);
        KdpPrintBanner(MemSizeMBs);

        /* Check if the debugger should be disabled initially */
        if (DisableKdAfterInit)
        {
            /* Disable it */
            KdDisableDebuggerWithLock(FALSE);

            /*
             * Save the enable block state and return initialized
             * (the debugger is active but disabled).
             */
            KdBlockEnable = BlockEnable;
            return TRUE;
        }

        /* Check if we have a loader block */
        if (LoaderBlock)
        {
            /* Loop boot images */
            NextEntry = LoaderBlock->LoadOrderListHead.Flink;
            i = 0;
            while ((NextEntry != &LoaderBlock->LoadOrderListHead) && (i < 2))
            {
                /* Get the image entry */
                LdrEntry = CONTAINING_RECORD(NextEntry,
                                             LDR_DATA_TABLE_ENTRY,
                                             InLoadOrderLinks);

                /* Generate the image name */
                Name = LdrEntry->FullDllName.Buffer;
                Length = LdrEntry->FullDllName.Length / sizeof(WCHAR);
                j = 0;
                do
                {
                    /* Do cheap Unicode to ANSI conversion */
                    NameBuffer[j++] = (CHAR)*Name++;
                } while (j < Length);

                /* Null-terminate */
                NameBuffer[j] = ANSI_NULL;

                /* Load symbols for image */
                RtlInitString(&ImageName, NameBuffer);
#ifdef _WINKD_
                DbgLoadImageSymbols(&ImageName,
                                    LdrEntry->DllBase,
                                    (ULONG_PTR)PsGetCurrentProcessId());
#endif

                /* Go to the next entry */
                NextEntry = NextEntry->Flink;
                i++;
            }
        }

        /* Check for incoming breakin and break on symbol load if we have it */
        KdBreakAfterSymbolLoad = KdPollBreakIn();
    }
    else
    {
        /* Disable debugger */
        KdDebuggerNotPresent = TRUE;
    }

    /* Return initialized */
    return TRUE;
}
