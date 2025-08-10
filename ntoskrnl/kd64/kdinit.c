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

#include <debug.h>

/*
 * Override DbgPrint(), used by the debugger banner DPRINTs below,
 * because KdInitSystem() can be called under the debugger lock by
 * KdEnableDebugger(WithLock)().
 */
#define DbgPrint(fmt, ...) (KdpDprintf(fmt, ##__VA_ARGS__), 0)
#define DbgPrintEx(cmpid, lvl, fmt, ...) (KdpDprintf(fmt, ##__VA_ARGS__), 0)

/* UTILITY FUNCTIONS *********************************************************/

#include <mm/ARM3/miarm.h> // For MiIsMemoryTypeInvisible()

/**
 * @brief
 * Retrieves the total size of the memory before Mm is initialized,
 * by counting the number of physical pages. Useful for debug logging.
 *
 * Adapted from mm/ARM3/mminit.c!MiScanMemoryDescriptors().
 **/
static
SIZE_T
KdpGetMemorySizeInMBs(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY ListEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    SIZE_T NumberOfPhysicalPages = 0;

    /*
     * If no loader block is present (e.g. the debugger is initialized only
     * much later after boot), just use the already-initialized Mm-computed
     * number of physical pages. Otherwise do the evaluation ourselves.
     */
    if (!LoaderBlock)
    {
        NumberOfPhysicalPages = MmNumberOfPhysicalPages;
        goto ReturnSize;
    }

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        /* If this is invisible memory, skip this descriptor */
        if (MiIsMemoryTypeInvisible(Descriptor->MemoryType))
            continue;

        /* Check if this isn't bad memory */
        if (Descriptor->MemoryType != LoaderBad)
        {
            /* Count it in the physical pages */
            NumberOfPhysicalPages += Descriptor->PageCount;
        }
    }

ReturnSize:
    /* Round size up. Assumed to better match actual physical RAM size */
    return ALIGN_UP_BY(NumberOfPhysicalPages * PAGE_SIZE, 1024 * 1024) / (1024 * 1024);
}

/**
 * @brief
 * Displays the kernel debugger initialization banner.
 **/
static
VOID
KdpPrintBanner(VOID)
{
    SIZE_T MemSizeMBs = KdpGetMemorySizeInMBs(KeLoaderBlock);

#ifdef _M_AMD64
    /* On AMD64 during early init, use direct serial output to avoid recursion */
    {
        const char msg1[] = "-----------------------------------------------------\n";
        const char *p = msg1;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        
        const char msg2[] = "ReactOS " KERNEL_VERSION_STR " (Build " KERNEL_VERSION_BUILD_STR ") (Commit " KERNEL_VERSION_COMMIT_HASH ")\n";
        p = msg2;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        
        /* Format and output processor/memory info - simplified version */
        const char msg3[] = "System Processor and Memory initialized\n";
        p = msg3;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        
        if (KeLoaderBlock)
        {
            const char msg4[] = "Loader block present\n";
            p = msg4;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
    }
#else
    DPRINT1("-----------------------------------------------------\n");
    DPRINT1("ReactOS " KERNEL_VERSION_STR " (Build " KERNEL_VERSION_BUILD_STR ") (Commit " KERNEL_VERSION_COMMIT_HASH ")\n");
    DPRINT1("%u System Processor [%u MB Memory]\n", KeNumberProcessors, MemSizeMBs);

    if (KeLoaderBlock)
    {
        DPRINT1("Command Line: %s\n", KeLoaderBlock->LoadOptions);
        DPRINT1("ARC Paths: %s %s %s %s\n",
                KeLoaderBlock->ArcBootDeviceName, KeLoaderBlock->NtHalPathName,
                KeLoaderBlock->ArcHalDeviceName, KeLoaderBlock->NtBootPathName);
    }
#endif
}

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KdUpdateDataBlock(VOID)
{
    /* Update the KeUserCallbackDispatcher pointer */
    KdDebuggerDataBlock.KeUserCallbackDispatcher =
        (ULONG_PTR)KeUserCallbackDispatcher;
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
KdInitSystem(
    _In_ ULONG BootPhase,
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    BOOLEAN EnableKd, DisableKdAfterInit = FALSE, BlockEnable = FALSE;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG i;

#ifdef _M_AMD64
    /* Debug output for AMD64 */
    {
        const char msg[] = "*** KdInitSystem: Entry ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
    }
#endif

    /* Check if this is Phase 1 */
    if (BootPhase)
    {
        /* Just query the performance counter */
        KeQueryPerformanceCounter(&KdPerformanceCounterRate);
        return TRUE;
    }

    /* Check if we already initialized once */
    if (KdDebuggerEnabled)
        return TRUE;

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
#ifdef _M_AMD64
        /* On AMD64 during early boot, GS might not be fully set up yet */
        /* Check if GS base is set by reading the MSR */
        {
            const char msg1[] = "*** KdInitSystem: About to read GS MSR ***\n";
            const char *p1 = msg1;
            while (*p1) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p1++); }
            
            PKPCR Pcr = NULL;
            ULONG64 GsBase = __readmsr(0xC0000101); /* MSR_GS_BASE */
            
            const char msg2[] = "*** KdInitSystem: GS MSR read completed ***\n";
            const char *p2 = msg2;
            while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
            
            if (GsBase != 0)
            {
                /* GS is set, we can use it */
                const char msg3[] = "*** KdInitSystem: Using GS base for PCR ***\n";
                const char *p3 = msg3;
                while (*p3) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p3++); }
                Pcr = (PKPCR)GsBase;
            }
            else if (LoaderBlock && LoaderBlock->Prcb)
            {
                /* GS not set up yet - use LoaderBlock */
                const char msg4[] = "*** KdInitSystem: Using LoaderBlock for PCR ***\n";
                const char *p4 = msg4;
                while (*p4) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p4++); }
                
                ULONG_PTR PrcbAddr = LoaderBlock->Prcb;
                if (PrcbAddr < 0xFFFF800000000000ULL)
                {
                    /* Convert physical to kernel VA */
                    PrcbAddr = PrcbAddr + 0xFFFF800000000000ULL;
                }
                /* On AMD64, the PCR starts 0x180 bytes before the PRCB */
                Pcr = (PKPCR)((ULONG_PTR)PrcbAddr - 0x180);
            }
            
            if (Pcr)
            {
                const char msg5[] = "*** KdInitSystem: Setting KdVersionBlock in PCR ***\n";
                const char *p5 = msg5;
                while (*p5) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p5++); }
                
                Pcr->KdVersionBlock = &KdVersionBlock;
                
                const char msg6[] = "*** KdInitSystem: KdVersionBlock set ***\n";
                const char *p6 = msg6;
                while (*p6) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p6++); }
            }
        }
#else
        KeGetPcr()->KdVersionBlock = &KdVersionBlock;
#endif
    }

    /* Check if we have a loader block */
    if (LoaderBlock)
    {
        PSTR CommandLine, DebugLine;

#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Processing LoaderBlock ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif

        /* Get the image entry */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Checking LoadOrderListHead ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            
            if (LoaderBlock->LoadOrderListHead.Flink == NULL)
            {
                const char msg2[] = "*** KdInitSystem: ERROR - LoadOrderListHead.Flink is NULL! ***\n";
                const char *p2 = msg2;
                while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
                
                /* Can't continue without module list */
                return TRUE;
            }
            
            const char msg3[] = "*** KdInitSystem: LoadOrderListHead.Flink is valid ***\n";
            const char *p3 = msg3;
            while (*p3) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p3++); }
        }
#endif
        LdrEntry = CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Got LdrEntry ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif

        /* Save the Kernel Base */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: About to access LdrEntry->DllBase ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            
            if (LdrEntry)
            {
                const char msg2[] = "*** KdInitSystem: LdrEntry is not NULL ***\n";
                const char *p2 = msg2;
                while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
                
                /* Try to access DllBase */
                PVOID DllBase = LdrEntry->DllBase;
                
                const char msg3[] = "*** KdInitSystem: DllBase accessed successfully ***\n";
                const char *p3 = msg3;
                while (*p3) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p3++); }
                
                PsNtosImageBase = (ULONG_PTR)DllBase;
                KdVersionBlock.KernBase = (ULONG64)(LONG_PTR)DllBase;
                
                const char msg4[] = "*** KdInitSystem: Kernel base saved ***\n";
                const char *p4 = msg4;
                while (*p4) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p4++); }
            }
            else
            {
                const char msg2[] = "*** KdInitSystem: ERROR - LdrEntry is NULL! ***\n";
                const char *p2 = msg2;
                while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
            }
        }
#else
        PsNtosImageBase = (ULONG_PTR)LdrEntry->DllBase;
        KdVersionBlock.KernBase = (ULONG64)(LONG_PTR)LdrEntry->DllBase;
#endif

        /* Check if we have a command line */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Checking LoadOptions ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        CommandLine = LoaderBlock->LoadOptions;
        if (CommandLine)
        {
#ifdef _M_AMD64
            {
                const char msg[] = "*** KdInitSystem: WARNING - Skipping command line processing on AMD64 ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            }
            /* On AMD64, LoadOptions might be in read-only memory from UEFI */
            /* Skip command line processing for now - just enable KD */
            EnableKd = TRUE;
            goto SkipCommandLine;
#else
            /* Upcase it */
            _strupr(CommandLine);
#endif
#ifdef _M_AMD64
            {
                const char msg[] = "*** KdInitSystem: _strupr completed ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            }
#endif

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
            else if ((DebugLine = strstr(CommandLine, "DEBUG")))
            {
                /* Enable KD */
                EnableKd = TRUE;

                /* Check if there are any options */
                if (DebugLine[5] == '=')
                {
                    /* Save pointers */
                    PSTR DebugOptionStart, DebugOptionEnd;
                    DebugOptionStart = DebugOptionEnd = &DebugLine[6];

                    /* Scan the string for debug options */
                    for (;;)
                    {
                        SIZE_T DebugOptionLength;

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
                                 * the end of the string, break out.
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
                         * or if there were no options at all.
                         */
                        if (!DebugOptionLength)
                            break;

                        /* Now check which option this is */
                        if ((DebugOptionLength == 10) &&
                            !(strncmp(DebugOptionStart, "AUTOENABLE", 10)))
                        {
                            /* Disable the debugger, but
                             * allow to re-enable it later */
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
                         * If there are more options then the next character
                         * should be a comma. Break out if it isn't.
                         */
                        if (*DebugOptionEnd != ',')
                            break;

                        /* Move on to the next option */
                        DebugOptionEnd++;
                        DebugOptionStart = DebugOptionEnd;
                    }
                }
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

#ifdef _M_AMD64
SkipCommandLine:
    {
        const char msg[] = "*** KdInitSystem: Command line processing complete/skipped ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
    }
#endif

    /* Set the Kernel Base in the Data Block */
#ifdef _M_AMD64
    {
        const char msg[] = "*** KdInitSystem: Setting KernBase in DataBlock ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
    }
#endif
    KdDebuggerDataBlock.KernBase = (ULONG_PTR)KdVersionBlock.KernBase;

#ifdef _M_AMD64
    {
        const char msg[] = "*** KdInitSystem: About to check EnableKd ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
    }
#endif

    /* Initialize the debugger if requested */
#ifdef _M_AMD64
    {
        const char msg1[] = "*** KdInitSystem: Checking EnableKd flag ***\n";
        const char *p1 = msg1;
        while (*p1) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p1++); }
        
        if (EnableKd)
        {
            const char msg2[] = "*** KdInitSystem: EnableKd is TRUE, calling KdDebuggerInitialize0 ***\n";
            const char *p2 = msg2;
            while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
        }
        else
        {
            const char msg2[] = "*** KdInitSystem: EnableKd is FALSE, skipping KdDebuggerInitialize0 ***\n";
            const char *p2 = msg2;
            while (*p2) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p2++); }
        }
    }
#endif
    if (EnableKd && (NT_SUCCESS(KdDebuggerInitialize0(LoaderBlock))))
    {
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: KdDebuggerInitialize0 succeeded ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        /* Now set our real KD routine */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Setting KiDebugRoutine ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        KiDebugRoutine = KdpTrap;

#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Checking KdpDebuggerStructuresInitialized ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        /* Check if we've already initialized our structures */
        if (!KdpDebuggerStructuresInitialized)
        {
            /* Set Retries */
            KdpContext.KdpDefaultRetries = 20;

            /* Initialize breakpoints owed flag and table */
#ifdef _M_AMD64
            {
                const char msg[] = "*** KdInitSystem: Initializing breakpoints ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            }
#endif
            KdpOweBreakpoint = FALSE;
            for (i = 0; i < KD_BREAKPOINT_MAX; i++)
            {
                KdpBreakpointTable[i].Flags   = 0;
                KdpBreakpointTable[i].DirectoryTableBase = 0;
                KdpBreakpointTable[i].Address = NULL;
            }

            /* Initialize the Time Slip DPC */
#ifdef _M_AMD64
            {
                const char msg[] = "*** KdInitSystem: SKIPPING DPC/Timer init on AMD64 (temporary) ***\n";
                const char *p = msg;
                while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
            }
            /* Skip DPC/Timer initialization on AMD64 for now - causes crash */
#else
            KeInitializeDpc(&KdpTimeSlipDpc, KdpTimeSlipDpcRoutine, NULL);
            KeInitializeTimer(&KdpTimeSlipTimer);
            ExInitializeWorkItem(&KdpTimeSlipWorkItem, KdpTimeSlipWork, NULL);
#endif

            /* First-time initialization done! */
            KdpDebuggerStructuresInitialized = TRUE;
        }

        /* Initialize the timer */
        KdTimerStart.QuadPart = 0;

        /* Officially enable KD */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Enabling KD ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        KdPitchDebugger = FALSE;
        KdDebuggerEnabled = TRUE;
        KdDebuggerNotPresent = FALSE; /* Debugger IS present */

        /* Let user-mode know that it's enabled as well */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Setting SharedUserData ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
        SharedUserData->KdDebuggerEnabled = TRUE;

        /* Display separator + ReactOS version at the start of the debug log */
        KdpPrintBanner();

        /* Check if the debugger should be disabled initially */
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: Checking DisableKdAfterInit ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
#endif
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
#ifdef _M_AMD64
        {
            const char msg[] = "*** KdInitSystem: SKIPPING symbol loading on AMD64 (temporary) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
        }
        /* Skip symbol loading on AMD64 for now */
#else
        if (LoaderBlock)
        {
            PLIST_ENTRY NextEntry;
            ULONG j, Length;
            PWCHAR Name;
            STRING ImageName;
            CHAR NameBuffer[256];

            /* Loop over the first two boot images: HAL and kernel */
            for (NextEntry = LoaderBlock->LoadOrderListHead.Flink, i = 0;
                 NextEntry != &LoaderBlock->LoadOrderListHead && (i < 2);
                 NextEntry = NextEntry->Flink, ++i)
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

                /* Load the symbols */
                RtlInitString(&ImageName, NameBuffer);
                DbgLoadImageSymbols(&ImageName,
                                    LdrEntry->DllBase,
                                    (ULONG_PTR)PsGetCurrentProcessId());
            }

            /* Check for incoming break-in and break on symbol load
             * if requested, see ex/init.c!ExpLoadBootSymbols() */
            KdBreakAfterSymbolLoad = KdPollBreakIn();
        }
#endif /* _M_AMD64 */
    }
    else
    {
        /* Disable debugger */
        KdDebuggerNotPresent = TRUE;
    }

    /* Return initialized */
#ifdef _M_AMD64
    {
        const char msg[] = "*** KdInitSystem: About to return TRUE ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(0x3F8 + 5) & 0x20) == 0); __outbyte(0x3F8, *p++); }
    }
#endif
    return TRUE;
}
