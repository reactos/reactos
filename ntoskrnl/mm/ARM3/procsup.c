/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/procsup.c
 * PURPOSE:         ARM Memory Manager Process Related Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

ULONG MmProcessColorSeed = 0x12345678;
ULONG MmMaximumDeadKernelStacks = 5;
SLIST_HEADER MmDeadStackSListHead;
ULONG MmRotatingUniprocessorNumber = 0;

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
MiCreatePebOrTeb(IN PEPROCESS Process,
                 IN ULONG Size,
                 OUT PULONG_PTR BaseAddress)
{
    PMMVAD_LONG Vad;
    NTSTATUS Status;
    ULONG_PTR HighestAddress, RandomBase;
    ULONG AlignedSize;
    LARGE_INTEGER CurrentTime;

    Status = PsChargeProcessNonPagedPoolQuota(Process, sizeof(MMVAD_LONG));
    if (!NT_SUCCESS(Status))
        return Status;

    /* Allocate a VAD */
    Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD_LONG), 'ldaV');
    if (!Vad)
    {
        Status = STATUS_NO_MEMORY;
        goto FailPath;
    }

    /* Setup the primary flags with the size, and make it commited, private, RW */
    Vad->u.LongFlags = 0;
    Vad->u.VadFlags.CommitCharge = BYTES_TO_PAGES(Size);
    Vad->u.VadFlags.MemCommit = TRUE;
    Vad->u.VadFlags.PrivateMemory = TRUE;
    Vad->u.VadFlags.Protection = MM_READWRITE;
    Vad->u.VadFlags.NoChange = TRUE;
    Vad->u1.Parent = NULL;

    /* Setup the secondary flags to make it a secured, writable, long VAD */
    Vad->u2.LongFlags2 = 0;
    Vad->u2.VadFlags2.OneSecured = TRUE;
    Vad->u2.VadFlags2.LongVad = TRUE;
    Vad->u2.VadFlags2.ReadOnly = FALSE;

    Vad->ControlArea = NULL; // For Memory-Area hack
    Vad->FirstPrototypePte = NULL;

    /* Check if this is a PEB creation */
    ASSERT(sizeof(TEB) != sizeof(PEB));
    if (Size == sizeof(PEB))
    {
        /* Create a random value to select one page in a 64k region */
        KeQueryTickCount(&CurrentTime);
        CurrentTime.LowPart &= (_64K / PAGE_SIZE) - 1;

        /* Calculate a random base address */
        RandomBase = (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1;
        RandomBase -= CurrentTime.LowPart << PAGE_SHIFT;

        /* Make sure the base address is not too high */
        AlignedSize = ROUND_TO_PAGES(Size);
        if ((RandomBase + AlignedSize) > (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1)
        {
            RandomBase = (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1 - AlignedSize;
        }

        /* Calculate the highest allowed address */
        HighestAddress = RandomBase + AlignedSize - 1;
    }
    else
    {
        HighestAddress = (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS;
    }

    *BaseAddress = 0;
    Status = MiInsertVadEx((PMMVAD)Vad,
                           BaseAddress,
                           Size,
                           HighestAddress,
                           PAGE_SIZE,
                           MEM_TOP_DOWN);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Vad, 'ldaV');
        Status = STATUS_NO_MEMORY;
        goto FailPath;
    }


    /* Success */
    return STATUS_SUCCESS;

FailPath:
    PsReturnProcessNonPagedPoolQuota(Process, sizeof(MMVAD_LONG));

    return Status;
}

VOID
NTAPI
MmDeleteTeb(IN PEPROCESS Process,
            IN PTEB Teb)
{
    ULONG_PTR TebEnd;
    PETHREAD Thread = PsGetCurrentThread();
    PMMVAD Vad;
    PMM_AVL_TABLE VadTree = &Process->VadRoot;
    DPRINT("Deleting TEB: %p in %16s\n", Teb, Process->ImageFileName);

    /* TEB is one page */
    TebEnd = (ULONG_PTR)Teb + ROUND_TO_PAGES(sizeof(TEB)) - 1;

    /* Attach to the process */
    KeAttachProcess(&Process->Pcb);

    /* Lock the process address space */
    KeAcquireGuardedMutex(&Process->AddressCreationLock);

    /* Find the VAD, make sure it's a TEB VAD */
    Vad = MiLocateAddress(Teb);
    DPRINT("Removing node for VAD: %lx %lx\n", Vad->StartingVpn, Vad->EndingVpn);
    ASSERT(Vad != NULL);
    if (Vad->StartingVpn != ((ULONG_PTR)Teb >> PAGE_SHIFT))
    {
        /* Bug in the AVL code? */
        DPRINT1("Corrupted VAD!\n");
    }
    else
    {
        /* Sanity checks for a valid TEB VAD */
        ASSERT((Vad->StartingVpn == ((ULONG_PTR)Teb >> PAGE_SHIFT) &&
               (Vad->EndingVpn == (TebEnd >> PAGE_SHIFT))));
        ASSERT(Vad->u.VadFlags.NoChange == TRUE);
        ASSERT(Vad->u2.VadFlags2.OneSecured == TRUE);
        ASSERT(Vad->u2.VadFlags2.MultipleSecured == FALSE);

        /* Lock the working set */
        MiLockProcessWorkingSetUnsafe(Process, Thread);

        /* Remove this VAD from the tree */
        ASSERT(VadTree->NumberGenericTableElements >= 1);
        MiRemoveNode((PMMADDRESS_NODE)Vad, VadTree);

        /* Delete the pages */
        MiDeleteVirtualAddresses((ULONG_PTR)Teb, TebEnd, NULL);

        /* Release the working set */
        MiUnlockProcessWorkingSetUnsafe(Process, Thread);

        /* Remove the VAD */
        ExFreePool(Vad);

        /* Return the quota the VAD used */
        PsReturnProcessNonPagedPoolQuota(Process, sizeof(MMVAD_LONG));
    }

    /* Release the address space lock */
    KeReleaseGuardedMutex(&Process->AddressCreationLock);

    /* Detach */
    KeDetachProcess();
}

VOID
NTAPI
MmDeleteKernelStack(IN PVOID StackBase,
                    IN BOOLEAN GuiStack)
{
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameNumber, PageTableFrameNumber;
    PFN_COUNT StackPages;
    PMMPFN Pfn1, Pfn2;
    ULONG i;
    KIRQL OldIrql;
    PSLIST_ENTRY SListEntry;

    //
    // This should be the guard page, so decrement by one
    //
    PointerPte = MiAddressToPte(StackBase);
    PointerPte--;

    //
    // If this is a small stack, just push the stack onto the dead stack S-LIST
    //
    if (!GuiStack)
    {
        if (ExQueryDepthSList(&MmDeadStackSListHead) < MmMaximumDeadKernelStacks)
        {
            SListEntry = ((PSLIST_ENTRY)StackBase) - 1;
            InterlockedPushEntrySList(&MmDeadStackSListHead, SListEntry);
            return;
        }
    }

    //
    // Calculate pages used
    //
    StackPages = BYTES_TO_PAGES(GuiStack ?
                                MmLargeStackSize : KERNEL_STACK_SIZE);

    /* Acquire the PFN lock */
    OldIrql = MiAcquirePfnLock();

    //
    // Loop them
    //
    for (i = 0; i < StackPages; i++)
    {
        //
        // Check if this is a valid PTE
        //
        if (PointerPte->u.Hard.Valid == 1)
        {
            /* Get the PTE's page */
            PageFrameNumber = PFN_FROM_PTE(PointerPte);
            Pfn1 = MiGetPfnEntry(PageFrameNumber);

            /* Now get the page of the page table mapping it */
            PageTableFrameNumber = Pfn1->u4.PteFrame;
            Pfn2 = MiGetPfnEntry(PageTableFrameNumber);

            /* Remove a shared reference, since the page is going away */
            MiDecrementShareCount(Pfn2, PageTableFrameNumber);

            /* Set the special pending delete marker */
            MI_SET_PFN_DELETED(Pfn1);

            /* And now delete the actual stack page */
            MiDecrementShareCount(Pfn1, PageFrameNumber);
        }

        //
        // Next one
        //
        PointerPte--;
    }

    //
    // We should be at the guard page now
    //
    ASSERT(PointerPte->u.Hard.Valid == 0);

    /* Release the PFN lock */
    MiReleasePfnLock(OldIrql);

    //
    // Release the PTEs
    //
    MiReleaseSystemPtes(PointerPte, StackPages + 1, SystemPteSpace);
}

PVOID
NTAPI
MmCreateKernelStack(IN BOOLEAN GuiStack,
                    IN UCHAR Node)
{
    PFN_COUNT StackPtes, StackPages;
    PMMPTE PointerPte, StackPte;
    PVOID BaseAddress;
    MMPTE TempPte, InvalidPte;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    ULONG i;
    PSLIST_ENTRY SListEntry;

    //
    // Calculate pages needed
    //
    if (GuiStack)
    {
        //
        // We'll allocate 64KB stack, but only commit 12K
        //
        StackPtes = BYTES_TO_PAGES(MmLargeStackSize);
        StackPages = BYTES_TO_PAGES(KERNEL_LARGE_STACK_COMMIT);
    }
    else
    {
        //
        // If the dead stack S-LIST has a stack on it, use it instead of allocating
        // new system PTEs for this stack
        //
        if (ExQueryDepthSList(&MmDeadStackSListHead))
        {
            SListEntry = InterlockedPopEntrySList(&MmDeadStackSListHead);
            if (SListEntry != NULL)
            {
                BaseAddress = (SListEntry + 1);
                return BaseAddress;
            }
        }

        //
        // We'll allocate 12K and that's it
        //
        StackPtes = BYTES_TO_PAGES(KERNEL_STACK_SIZE);
        StackPages = StackPtes;
    }

    //
    // Reserve stack pages, plus a guard page
    //
    StackPte = MiReserveSystemPtes(StackPtes + 1, SystemPteSpace);
    if (!StackPte) return NULL;

    //
    // Get the stack address
    //
    BaseAddress = MiPteToAddress(StackPte + StackPtes + 1);

    //
    // Select the right PTE address where we actually start committing pages
    //
    PointerPte = StackPte;
    if (GuiStack) PointerPte += BYTES_TO_PAGES(MmLargeStackSize -
                                               KERNEL_LARGE_STACK_COMMIT);


    /* Setup the temporary invalid PTE */
    MI_MAKE_SOFTWARE_PTE(&InvalidPte, MM_NOACCESS);

    /* Setup the template stack PTE */
    MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte, PointerPte + 1, MM_READWRITE, 0);

    //
    // Acquire the PFN DB lock
    //
    OldIrql = MiAcquirePfnLock();

    //
    // Loop each stack page
    //
    for (i = 0; i < StackPages; i++)
    {
        //
        // Next PTE
        //
        PointerPte++;

        /* Get a page and write the current invalid PTE */
        MI_SET_USAGE(MI_USAGE_KERNEL_STACK);
        MI_SET_PROCESS2(PsGetCurrentProcess()->ImageFileName);
        PageFrameIndex = MiRemoveAnyPage(MI_GET_NEXT_COLOR());
        MI_WRITE_INVALID_PTE(PointerPte, InvalidPte);

        /* Initialize the PFN entry for this page */
        MiInitializePfn(PageFrameIndex, PointerPte, 1);

        /* Write the valid PTE */
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE(PointerPte, TempPte);
    }

    //
    // Release the PFN lock
    //
    MiReleasePfnLock(OldIrql);

    //
    // Return the stack address
    //
    return BaseAddress;
}

NTSTATUS
NTAPI
MmGrowKernelStackEx(IN PVOID StackPointer,
                    IN ULONG GrowSize)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PMMPTE LimitPte, NewLimitPte, LastPte;
    KIRQL OldIrql;
    MMPTE TempPte, InvalidPte;
    PFN_NUMBER PageFrameIndex;

    //
    // Make sure the stack did not overflow
    //
    ASSERT(((ULONG_PTR)Thread->StackBase - (ULONG_PTR)Thread->StackLimit) <=
           (MmLargeStackSize + PAGE_SIZE));

    //
    // Get the current stack limit
    //
    LimitPte = MiAddressToPte(Thread->StackLimit);
    ASSERT(LimitPte->u.Hard.Valid == 1);

    //
    // Get the new one and make sure this isn't a retarded request
    //
    NewLimitPte = MiAddressToPte((PVOID)((ULONG_PTR)StackPointer - GrowSize));
    if (NewLimitPte == LimitPte) return STATUS_SUCCESS;

    //
    // Now make sure you're not going past the reserved space
    //
    LastPte = MiAddressToPte((PVOID)((ULONG_PTR)Thread->StackBase -
                                     MmLargeStackSize));
    if (NewLimitPte < LastPte)
    {
        //
        // Sorry!
        //
        return STATUS_STACK_OVERFLOW;
    }

    //
    // Calculate the number of new pages
    //
    LimitPte--;

    /* Setup the temporary invalid PTE */
    MI_MAKE_SOFTWARE_PTE(&InvalidPte, MM_NOACCESS);

    //
    // Acquire the PFN DB lock
    //
    OldIrql = MiAcquirePfnLock();

    //
    // Loop each stack page
    //
    while (LimitPte >= NewLimitPte)
    {
        /* Get a page and write the current invalid PTE */
        MI_SET_USAGE(MI_USAGE_KERNEL_STACK_EXPANSION);
        MI_SET_PROCESS2(PsGetCurrentProcess()->ImageFileName);
        PageFrameIndex = MiRemoveAnyPage(MI_GET_NEXT_COLOR());
        MI_WRITE_INVALID_PTE(LimitPte, InvalidPte);

        /* Initialize the PFN entry for this page */
        MiInitializePfn(PageFrameIndex, LimitPte, 1);

        /* Setup the template stack PTE */
        MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte, LimitPte, MM_READWRITE, PageFrameIndex);

        /* Write the valid PTE */
        MI_WRITE_VALID_PTE(LimitPte--, TempPte);
    }

    //
    // Release the PFN lock
    //
    MiReleasePfnLock(OldIrql);

    //
    // Set the new limit
    //
    Thread->StackLimit = (ULONG_PTR)MiPteToAddress(NewLimitPte);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmGrowKernelStack(IN PVOID StackPointer)
{
    //
    // Call the extended version
    //
    return MmGrowKernelStackEx(StackPointer, KERNEL_LARGE_STACK_COMMIT);
}

NTSTATUS
NTAPI
MmSetMemoryPriorityProcess(IN PEPROCESS Process,
                           IN UCHAR MemoryPriority)
{
    UCHAR OldPriority;

    //
    // Check if we have less then 16MB of Physical Memory
    //
    if ((MmSystemSize == MmSmallSystem) &&
        (MmNumberOfPhysicalPages < ((15 * 1024 * 1024) / PAGE_SIZE)))
    {
        //
        // Always use background priority
        //
        MemoryPriority = MEMORY_PRIORITY_BACKGROUND;
    }

    //
    // Save the old priority and update it
    //
    OldPriority = (UCHAR)Process->Vm.Flags.MemoryPriority;
    Process->Vm.Flags.MemoryPriority = MemoryPriority;

    //
    // Return the old priority
    //
    return OldPriority;
}

NTSTATUS
NTAPI
MmCreatePeb(IN PEPROCESS Process,
            IN PINITIAL_PEB InitialPeb,
            OUT PPEB *BasePeb)
{
    PPEB Peb = NULL;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize = 0;
    PVOID TableBase = NULL;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    NTSTATUS Status;
    USHORT Characteristics;
    SectionOffset.QuadPart = (ULONGLONG)0;
    *BasePeb = NULL;

    //
    // Attach to Process
    //
    KeAttachProcess(&Process->Pcb);

    //
    // Map NLS Tables
    //
    Status = MmMapViewOfSection(ExpNlsSectionPointer,
                                (PEPROCESS)Process,
                                &TableBase,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                MEM_TOP_DOWN,
                                PAGE_READONLY);
    DPRINT("NLS Tables at: %p\n", TableBase);
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup and exit */
        KeDetachProcess();
        return Status;
    }

    //
    // Allocate the PEB
    //
    Status = MiCreatePebOrTeb(Process, sizeof(PEB), (PULONG_PTR)&Peb);
    DPRINT("PEB at: %p\n", Peb);
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup and exit */
        KeDetachProcess();
        return Status;
    }

    //
    // Use SEH in case we can't load the PEB
    //
    _SEH2_TRY
    {
        //
        // Initialize the PEB
        //
        RtlZeroMemory(Peb, sizeof(PEB));

        //
        // Set up data
        //
        Peb->ImageBaseAddress = Process->SectionBaseAddress;
        Peb->InheritedAddressSpace = InitialPeb->InheritedAddressSpace;
        Peb->Mutant = InitialPeb->Mutant;
        Peb->ImageUsesLargePages = InitialPeb->ImageUsesLargePages;

        //
        // NLS
        //
        Peb->AnsiCodePageData = (PCHAR)TableBase + ExpAnsiCodePageDataOffset;
        Peb->OemCodePageData = (PCHAR)TableBase + ExpOemCodePageDataOffset;
        Peb->UnicodeCaseTableData = (PCHAR)TableBase + ExpUnicodeCaseTableDataOffset;

        //
        // Default Version Data (could get changed below)
        //
        Peb->OSMajorVersion = NtMajorVersion;
        Peb->OSMinorVersion = NtMinorVersion;
        Peb->OSBuildNumber = (USHORT)(NtBuildNumber & 0x3FFF);
        Peb->OSPlatformId = VER_PLATFORM_WIN32_NT;
        Peb->OSCSDVersion = (USHORT)CmNtCSDVersion;

        //
        // Heap and Debug Data
        //
        Peb->NumberOfProcessors = KeNumberProcessors;
        Peb->ImageProcessAffinityMask = KeActiveProcessors;
        Peb->BeingDebugged = (BOOLEAN)(Process->DebugPort != NULL);
        Peb->NtGlobalFlag = NtGlobalFlag;
        Peb->HeapSegmentReserve = MmHeapSegmentReserve;
        Peb->HeapSegmentCommit = MmHeapSegmentCommit;
        Peb->HeapDeCommitTotalFreeThreshold = MmHeapDeCommitTotalFreeThreshold;
        Peb->HeapDeCommitFreeBlockThreshold = MmHeapDeCommitFreeBlockThreshold;
        Peb->CriticalSectionTimeout = MmCriticalSectionTimeout;
        Peb->MinimumStackCommit = MmMinimumStackCommitInBytes;
        Peb->MaximumNumberOfHeaps = (PAGE_SIZE - sizeof(PEB)) / sizeof(PVOID);
        Peb->ProcessHeaps = (PVOID*)(Peb + 1);

        //
        // Session ID
        //
        if (Process->Session) Peb->SessionId = MmGetSessionId(Process);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Fail
        //
        KeDetachProcess();
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    //
    // Use SEH in case we can't load the image
    //
    _SEH2_TRY
    {
        //
        // Get NT Headers
        //
        NtHeaders = RtlImageNtHeader(Peb->ImageBaseAddress);
        Characteristics = NtHeaders->FileHeader.Characteristics;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Fail
        //
        KeDetachProcess();
        _SEH2_YIELD(return STATUS_INVALID_IMAGE_PROTECT);
    }
    _SEH2_END;

    //
    // Parse the headers
    //
    if (NtHeaders)
    {
        //
        // Use SEH in case we can't load the headers
        //
        _SEH2_TRY
        {
            //
            // Get the Image Config Data too
            //
            ImageConfigData = RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                                           TRUE,
                                                           IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                           (PULONG)&ViewSize);
            if (ImageConfigData)
            {
                //
                // Probe it
                //
                ProbeForRead(ImageConfigData,
                             sizeof(IMAGE_LOAD_CONFIG_DIRECTORY),
                             sizeof(ULONG));
            }

            //
            // Write subsystem data
            //
            Peb->ImageSubsystem = NtHeaders->OptionalHeader.Subsystem;
            Peb->ImageSubsystemMajorVersion = NtHeaders->OptionalHeader.MajorSubsystemVersion;
            Peb->ImageSubsystemMinorVersion = NtHeaders->OptionalHeader.MinorSubsystemVersion;

            //
            // Check for version data
            //
            if (NtHeaders->OptionalHeader.Win32VersionValue)
            {
                //
                // Extract values and write them
                //
                Peb->OSMajorVersion = NtHeaders->OptionalHeader.Win32VersionValue & 0xFF;
                Peb->OSMinorVersion = (NtHeaders->OptionalHeader.Win32VersionValue >> 8) & 0xFF;
                Peb->OSBuildNumber = (NtHeaders->OptionalHeader.Win32VersionValue >> 16) & 0x3FFF;
                Peb->OSPlatformId = (NtHeaders->OptionalHeader.Win32VersionValue >> 30) ^ 2;

                /* Process CSD version override */
                if ((ImageConfigData) && (ImageConfigData->CSDVersion))
                {
                    /* Take the value from the image configuration directory */
                    Peb->OSCSDVersion = ImageConfigData->CSDVersion;
                }
            }

            /* Process optional process affinity mask override */
            if ((ImageConfigData) && (ImageConfigData->ProcessAffinityMask))
            {
                /* Take the value from the image configuration directory */
                Peb->ImageProcessAffinityMask = ImageConfigData->ProcessAffinityMask;
            }

            //
            // Check if this is a UP image
            if (Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)
            {
                //
                // Set single processor rotating affinity.
                // See https://www.microsoftpressstore.com/articles/article.aspx?p=2233328&seqNum=3
                //
                Peb->ImageProcessAffinityMask = AFFINITY_MASK(MmRotatingUniprocessorNumber);
                ASSERT(Peb->ImageProcessAffinityMask & KeActiveProcessors);
                MmRotatingUniprocessorNumber = (MmRotatingUniprocessorNumber + 1) % KeNumberProcessors;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Fail
            //
            KeDetachProcess();
            _SEH2_YIELD(return STATUS_INVALID_IMAGE_PROTECT);
        }
        _SEH2_END;
    }

    //
    // Detach from the Process
    //
    KeDetachProcess();
    *BasePeb = Peb;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmCreateTeb(IN PEPROCESS Process,
            IN PCLIENT_ID ClientId,
            IN PINITIAL_TEB InitialTeb,
            OUT PTEB *BaseTeb)
{
    PTEB Teb;
    NTSTATUS Status = STATUS_SUCCESS;
    *BaseTeb = NULL;

    //
    // Attach to Target
    //
    KeAttachProcess(&Process->Pcb);

    //
    // Allocate the TEB
    //
    Status = MiCreatePebOrTeb(Process, sizeof(TEB), (PULONG_PTR)&Teb);
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup and exit */
        KeDetachProcess();
        return Status;
    }

    //
    // Use SEH in case we can't load the TEB
    //
    _SEH2_TRY
    {
        //
        // Initialize the PEB
        //
        RtlZeroMemory(Teb, sizeof(TEB));

        //
        // Set TIB Data
        //
#ifdef _M_AMD64
        Teb->NtTib.ExceptionList = NULL;
#else
        Teb->NtTib.ExceptionList = EXCEPTION_CHAIN_END;
#endif
        Teb->NtTib.Self = (PNT_TIB)Teb;

        //
        // Identify this as an OS/2 V3.0 ("Cruiser") TIB
        //
        Teb->NtTib.Version = 30 << 8;

        //
        // Set TEB Data
        //
        Teb->ClientId = *ClientId;
        Teb->RealClientId = *ClientId;
        Teb->ProcessEnvironmentBlock = Process->Peb;
        Teb->CurrentLocale = PsDefaultThreadLocaleId;

        //
        // Check if we have a grandparent TEB
        //
        if ((InitialTeb->PreviousStackBase == NULL) &&
            (InitialTeb->PreviousStackLimit == NULL))
        {
            //
            // Use initial TEB values
            //
            Teb->NtTib.StackBase = InitialTeb->StackBase;
            Teb->NtTib.StackLimit = InitialTeb->StackLimit;
            Teb->DeallocationStack = InitialTeb->AllocatedStackBase;
        }
        else
        {
            //
            // Use grandparent TEB values
            //
            Teb->NtTib.StackBase = InitialTeb->PreviousStackBase;
            Teb->NtTib.StackLimit = InitialTeb->PreviousStackLimit;
        }

        //
        // Initialize the static unicode string
        //
        Teb->StaticUnicodeString.MaximumLength = sizeof(Teb->StaticUnicodeBuffer);
        Teb->StaticUnicodeString.Buffer = Teb->StaticUnicodeBuffer;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get error code
        //
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    //
    // Return
    //
    KeDetachProcess();
    *BaseTeb = Teb;
    return Status;
}

#ifdef _M_AMD64
static
NTSTATUS
MiInsertSharedUserPageVad(
    _In_ PEPROCESS Process)
{
    PMMVAD_LONG Vad;
    ULONG_PTR BaseAddress;
    NTSTATUS Status;

    if (Process->QuotaBlock != NULL)
    {
        Status = PsChargeProcessNonPagedPoolQuota(Process, sizeof(MMVAD_LONG));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Ran out of quota.\n");
            return Status;
        }
    }

    /* Allocate a VAD */
    Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD_LONG), 'ldaV');
    if (Vad == NULL)
    {
        DPRINT1("Failed to allocate VAD for shared user page\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FailPath;
    }

    /* Setup the primary flags with the size, and make it private, RO */
    Vad->u.LongFlags = 0;
    Vad->u.VadFlags.CommitCharge = 0;
    Vad->u.VadFlags.NoChange = TRUE;
    Vad->u.VadFlags.VadType = VadNone;
    Vad->u.VadFlags.MemCommit = FALSE;
    Vad->u.VadFlags.Protection = MM_READONLY;
    Vad->u.VadFlags.PrivateMemory = TRUE;
    Vad->u1.Parent = NULL;

    /* Setup the secondary flags to make it a secured, readonly, long VAD */
    Vad->u2.LongFlags2 = 0;
    Vad->u2.VadFlags2.OneSecured = TRUE;
    Vad->u2.VadFlags2.LongVad = TRUE;
    Vad->u2.VadFlags2.ReadOnly = FALSE;

    Vad->ControlArea = NULL; // For Memory-Area hack
    Vad->FirstPrototypePte = NULL;

    /* Insert it into the process VAD table */
    BaseAddress = MM_SHARED_USER_DATA_VA;
    Status = MiInsertVadEx((PMMVAD)Vad,
                            &BaseAddress,
                            PAGE_SIZE,
                            (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS,
                            PAGE_SIZE,
                            MEM_TOP_DOWN);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to insert shared user VAD\n");
        ExFreePoolWithTag(Vad, 'ldaV');
        goto FailPath;
    }

    /* Success */
    return STATUS_SUCCESS;

FailPath:
    if (Process->QuotaBlock != NULL)
        PsReturnProcessNonPagedPoolQuota(Process, sizeof(MMVAD_LONG));

    return Status;
}
#endif

NTSTATUS
NTAPI
MmInitializeProcessAddressSpace(IN PEPROCESS Process,
                                IN PEPROCESS ProcessClone OPTIONAL,
                                IN PVOID Section OPTIONAL,
                                IN OUT PULONG Flags,
                                IN POBJECT_NAME_INFORMATION *AuditName OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T ViewSize = 0;
    PVOID ImageBase = 0;
    PMMPTE PointerPte;
    KIRQL OldIrql;
    PMMPDE PointerPde;
    PMMPFN Pfn;
    PFN_NUMBER PageFrameNumber;
    UNICODE_STRING FileName;
    PWCHAR Source;
    PCHAR Destination;
    USHORT Length = 0;

#if (_MI_PAGING_LEVELS >= 3)
    PMMPPE PointerPpe;
#endif
#if (_MI_PAGING_LEVELS == 4)
    PMMPXE PointerPxe;
#endif

    /* We should have a PDE */
    ASSERT(Process->Pcb.DirectoryTableBase[0] != 0);
    ASSERT(Process->PdeUpdateNeeded == FALSE);

    /* Attach to the process */
    KeAttachProcess(&Process->Pcb);

    /* The address space should now been in phase 1 or 0 */
    ASSERT(Process->AddressSpaceInitialized <= 1);
    Process->AddressSpaceInitialized = 2;

    /* Initialize the Addresss Space lock */
    KeInitializeGuardedMutex(&Process->AddressCreationLock);
    Process->Vm.WorkingSetExpansionLinks.Flink = NULL;

    /* Initialize AVL tree */
    ASSERT(Process->VadRoot.NumberGenericTableElements == 0);
    Process->VadRoot.BalancedRoot.u1.Parent = &Process->VadRoot.BalancedRoot;

    /* Lock our working set */
    MiLockProcessWorkingSet(Process, PsGetCurrentThread());

    /* Lock PFN database */
    OldIrql = MiAcquirePfnLock();

    /* Setup the PFN for the PDE base of this process */
#if (_MI_PAGING_LEVELS == 4)
    PointerPte = MiAddressToPte(PXE_BASE);
#elif (_MI_PAGING_LEVELS == 3)
    PointerPte = MiAddressToPte(PPE_BASE);
#else
    PointerPte = MiAddressToPte(PDE_BASE);
#endif
    PageFrameNumber = PFN_FROM_PTE(PointerPte);
    ASSERT(Process->Pcb.DirectoryTableBase[0] == PageFrameNumber * PAGE_SIZE);
    MiInitializePfn(PageFrameNumber, PointerPte, TRUE);

    /* Do the same for hyperspace */
    PointerPde = MiAddressToPde(HYPER_SPACE);
    PageFrameNumber = PFN_FROM_PTE(PointerPde);
    MiInitializePfn(PageFrameNumber, (PMMPTE)PointerPde, TRUE);
#if (_MI_PAGING_LEVELS == 2)
    ASSERT(Process->Pcb.DirectoryTableBase[1] == PageFrameNumber * PAGE_SIZE);
#endif

#if (_MI_PAGING_LEVELS >= 3)
    PointerPpe = MiAddressToPpe((PVOID)HYPER_SPACE);
    PageFrameNumber = PFN_FROM_PTE(PointerPpe);
    MiInitializePfn(PageFrameNumber, PointerPpe, TRUE);
#if (_MI_PAGING_LEVELS == 3)
    ASSERT(Process->Pcb.DirectoryTableBase[1] == PageFrameNumber * PAGE_SIZE);
#endif
#endif
#if (_MI_PAGING_LEVELS == 4)
    PointerPxe = MiAddressToPxe((PVOID)HYPER_SPACE);
    PageFrameNumber = PFN_FROM_PTE(PointerPxe);
    MiInitializePfn(PageFrameNumber, PointerPxe, TRUE);
    ASSERT(Process->Pcb.DirectoryTableBase[1] == PageFrameNumber * PAGE_SIZE);
#endif

    /* Do the same for the Working set list */
    PointerPte = MiAddressToPte(MmWorkingSetList);
    PageFrameNumber = PFN_FROM_PTE(PointerPte);
    MiInitializePfn(PageFrameNumber, PointerPte, TRUE);

    /* All our pages are now active & valid. Release the lock. */
    MiReleasePfnLock(OldIrql);

    /* This should be in hyper space, but not in the mapping range */
    Process->Vm.VmWorkingSetList = MmWorkingSetList;
    ASSERT(((ULONG_PTR)MmWorkingSetList >= MI_MAPPING_RANGE_END) && ((ULONG_PTR)MmWorkingSetList <= HYPER_SPACE_END));

    /* Now initialize the working set list */
    MiInitializeWorkingSetList(&Process->Vm);

    /* The rule is that the owner process is always in the FLINK of the PDE's PFN entry */
    Pfn = MiGetPfnEntry(Process->Pcb.DirectoryTableBase[0] >> PAGE_SHIFT);
    ASSERT(Pfn->u4.PteFrame == MiGetPfnEntryIndex(Pfn));
    ASSERT(Pfn->u1.WsIndex == 0);
    Pfn->u1.Event = (PKEVENT)Process;

    /* Sanity check */
    ASSERT(Process->PhysicalVadRoot == NULL);

    /* Release the process working set */
    MiUnlockProcessWorkingSet(Process, PsGetCurrentThread());

#ifdef _M_AMD64
    /* On x64 we need a VAD for the shared user page */
    Status = MiInsertSharedUserPageVad(Process);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MiCreateSharedUserPageVad() failed: 0x%lx\n", Status);
        return Status;
    }
#endif

    /* Check if there's a Section Object */
    if (Section)
    {
        /* Determine the image file name and save it to EPROCESS */
        PFILE_OBJECT FileObject = MmGetFileObjectForSection(Section);
        FileName = FileObject->FileName;
        Source = (PWCHAR)((PCHAR)FileName.Buffer + FileName.Length);
        if (FileName.Buffer)
        {
            /* Loop the file name*/
            while (Source > FileName.Buffer)
            {
                /* Make sure this isn't a backslash */
                if (*--Source == OBJ_NAME_PATH_SEPARATOR)
                {
                    /* If so, stop it here */
                    Source++;
                    break;
                }
                else
                {
                    /* Otherwise, keep going */
                    Length++;
                }
            }
        }

        /* Copy the to the process and truncate it to 15 characters if necessary */
        Destination = Process->ImageFileName;
        Length = min(Length, sizeof(Process->ImageFileName) - 1);
        while (Length--) *Destination++ = (UCHAR)*Source++;
        *Destination = ANSI_NULL;

        /* Check if caller wants an audit name */
        if (AuditName)
        {
            /* Setup the audit name */
            Status = SeInitializeProcessAuditName(FileObject, FALSE, AuditName);
            if (!NT_SUCCESS(Status))
            {
                /* Fail */
                KeDetachProcess();
                return Status;
            }
        }

        /* Map the section */
        Status = MmMapViewOfSection(Section,
                                    Process,
                                    (PVOID*)&ImageBase,
                                    0,
                                    0,
                                    NULL,
                                    &ViewSize,
                                    ViewUnmap,
                                    MEM_COMMIT,
                                    PAGE_READWRITE);

        /* Save the pointer */
        Process->SectionBaseAddress = ImageBase;
    }

    /* Be nice and detach */
    KeDetachProcess();

    /* Return status to caller */
    return Status;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(IN PEPROCESS Process,
                             IN PULONG_PTR DirectoryTableBase)
{
    /* Share the directory base with the idle process */
    DirectoryTableBase[0] = PsGetCurrentProcess()->Pcb.DirectoryTableBase[0];
    DirectoryTableBase[1] = PsGetCurrentProcess()->Pcb.DirectoryTableBase[1];

    /* Initialize the Addresss Space */
    KeInitializeGuardedMutex(&Process->AddressCreationLock);
    KeInitializeSpinLock(&Process->HyperSpaceLock);
    Process->Vm.WorkingSetExpansionLinks.Flink = NULL;
    ASSERT(Process->VadRoot.NumberGenericTableElements == 0);
    Process->VadRoot.BalancedRoot.u1.Parent = &Process->VadRoot.BalancedRoot;

    /* Use idle process Working set */
    Process->Vm.VmWorkingSetList = PsGetCurrentProcess()->Vm.VmWorkingSetList;
    Process->WorkingSetPage = PsGetCurrentProcess()->WorkingSetPage;

    /* Done */
    Process->HasAddressSpace = TRUE;//??
    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
MmInitializeHandBuiltProcess2(IN PEPROCESS Process)
{
    /* Lock the VAD, ARM3-owned ranges away */
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            OUT PULONG_PTR DirectoryTableBase)
{
    KIRQL OldIrql;
    PFN_NUMBER TableBaseIndex, HyperIndex, WsListIndex;
    ULONG Color;

    /* Make sure we don't already have a page directory setup */
    ASSERT(Process->Pcb.DirectoryTableBase[0] == 0);
    ASSERT(Process->Pcb.DirectoryTableBase[1] == 0);
    ASSERT(Process->WorkingSetPage == 0);

    /* Choose a process color */
    Process->NextPageColor = (USHORT)RtlRandom(&MmProcessColorSeed);

    /* Setup the hyperspace lock */
    KeInitializeSpinLock(&Process->HyperSpaceLock);

    /* Lock PFN database */
    OldIrql = MiAcquirePfnLock();

    /*
     * Get a page for the table base, one for hyper space & one for the working set list.
     * The PFNs for these pages will be initialized in MmInitializeProcessAddressSpace,
     * when we are already attached to the process.
     * The other pages (if any) are allocated in the arch-specific part.
     */
    Color = MI_GET_NEXT_PROCESS_COLOR(Process);
    MI_SET_USAGE(MI_USAGE_PAGE_DIRECTORY);
    TableBaseIndex = MiRemoveZeroPageSafe(Color);
    if (!TableBaseIndex)
    {
        /* No zero pages, grab a free one */
        TableBaseIndex = MiRemoveAnyPage(Color);

        /* Zero it outside the PFN lock */
        MiReleasePfnLock(OldIrql);
        MiZeroPhysicalPage(TableBaseIndex);
        OldIrql = MiAcquirePfnLock();
    }
    MI_SET_USAGE(MI_USAGE_PAGE_DIRECTORY);
    Color = MI_GET_NEXT_PROCESS_COLOR(Process);
    HyperIndex = MiRemoveZeroPageSafe(Color);
    if (!HyperIndex)
    {
        /* No zero pages, grab a free one */
        HyperIndex = MiRemoveAnyPage(Color);

        /* Zero it outside the PFN lock */
        MiReleasePfnLock(OldIrql);
        MiZeroPhysicalPage(HyperIndex);
        OldIrql = MiAcquirePfnLock();
    }
    MI_SET_USAGE(MI_USAGE_PAGE_TABLE);
    Color = MI_GET_NEXT_PROCESS_COLOR(Process);
    WsListIndex = MiRemoveZeroPageSafe(Color);
    if (!WsListIndex)
    {
        /* No zero pages, grab a free one */
        WsListIndex = MiRemoveAnyPage(Color);

        /* Zero it outside the PFN lock */
        MiReleasePfnLock(OldIrql);
        MiZeroPhysicalPage(WsListIndex);
    }
    else
    {
        /* Release the PFN lock */
        MiReleasePfnLock(OldIrql);
    }

    /* Set the base directory pointers */
    Process->WorkingSetPage = WsListIndex;
    DirectoryTableBase[0] = TableBaseIndex << PAGE_SHIFT;
    DirectoryTableBase[1] = HyperIndex << PAGE_SHIFT;

    /* Perform the arch-specific parts */
    if (!MiArchCreateProcessAddressSpace(Process, DirectoryTableBase))
    {
        OldIrql = MiAcquirePfnLock();
        MiInsertPageInFreeList(WsListIndex);
        MiInsertPageInFreeList(HyperIndex);
        MiInsertPageInFreeList(TableBaseIndex);
        MiReleasePfnLock(OldIrql);
        Process->WorkingSetPage = 0;
        DirectoryTableBase[0] = 0;
        DirectoryTableBase[1] = 0;
        return FALSE;
    }

    /* Switch to phase 1 initialization */
    ASSERT(Process->AddressSpaceInitialized == 0);
    Process->AddressSpaceInitialized = 1;

    /* Add the process to the session */
    MiSessionAddProcess(Process);
    return TRUE;
}

VOID
NTAPI
MmCleanProcessAddressSpace(IN PEPROCESS Process)
{
    PMMVAD Vad;
    PMM_AVL_TABLE VadTree;
    PETHREAD Thread = PsGetCurrentThread();

    /* Remove from the session */
    MiSessionRemoveProcess();

    /* Abort early, when the address space wasn't fully initialized */
    if (Process->AddressSpaceInitialized < 2)
    {
        DPRINT1("Incomplete address space for Process %p. Might leak resources.\n",
                Process);
        return;
    }

    /* Lock the process address space from changes */
    MmLockAddressSpace(&Process->Vm);
    MiLockProcessWorkingSetUnsafe(Process, Thread);

    /* VM is deleted now */
    Process->VmDeleted = TRUE;
    MiUnlockProcessWorkingSetUnsafe(Process, Thread);

    /* Enumerate the VADs */
    VadTree = &Process->VadRoot;
    while (VadTree->NumberGenericTableElements)
    {
        /* Grab the current VAD */
        Vad = (PMMVAD)VadTree->BalancedRoot.RightChild;

        /* Check for old-style memory areas */
        if (Vad->u.VadFlags.Spare == 1)
        {
            /* Let RosMm handle this */
            MiRosCleanupMemoryArea(Process, Vad);
            continue;
        }

        /* Lock the working set */
        MiLockProcessWorkingSetUnsafe(Process, Thread);

        /* Remove this VAD from the tree */
        ASSERT(VadTree->NumberGenericTableElements >= 1);
        MiRemoveNode((PMMADDRESS_NODE)Vad, VadTree);

        /* Only regular VADs supported for now */
        ASSERT(Vad->u.VadFlags.VadType == VadNone);

        /* Check if this is a section VAD */
        if (!(Vad->u.VadFlags.PrivateMemory) && (Vad->ControlArea))
        {
            /* Remove the view */
            MiRemoveMappedView(Process, Vad);
        }
        else
        {
            /* Delete the addresses */
            MiDeleteVirtualAddresses(Vad->StartingVpn << PAGE_SHIFT,
                                     (Vad->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1),
                                     Vad);

            /* Release the working set */
            MiUnlockProcessWorkingSetUnsafe(Process, Thread);
        }

         /* Skip ARM3 fake VADs, they'll be freed by MmDeleteProcessAddresSpace */
        if (Vad->u.VadFlags.Spare == 1)
        {
            /* Set a flag so MmDeleteMemoryArea knows to free, but not to remove */
            Vad->u.VadFlags.Spare = 2;
            continue;
        }

        /* Free the VAD memory */
        ExFreePool(Vad);

        /* Return the quota the VAD used */
        PsReturnProcessNonPagedPoolQuota(Process, sizeof(MMVAD_LONG));
    }

    /* Lock the working set */
    MiLockProcessWorkingSetUnsafe(Process, Thread);
    ASSERT(Process->CloneRoot == NULL);
    ASSERT(Process->PhysicalVadRoot == NULL);

    /* Delete the shared user data section */
    MiDeleteVirtualAddresses(USER_SHARED_DATA, USER_SHARED_DATA, NULL);

    /* Release the working set */
    MiUnlockProcessWorkingSetUnsafe(Process, Thread);

    /* Release the address space */
    MmUnlockAddressSpace(&Process->Vm);
}

VOID
NTAPI
MmDeleteProcessAddressSpace(IN PEPROCESS Process)
{
    PMMPFN Pfn1, Pfn2;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;

#ifndef _M_AMD64
    OldIrql = MiAcquireExpansionLock();
    RemoveEntryList(&Process->MmProcessLinks);
    MiReleaseExpansionLock(OldIrql);
#endif

    //ASSERT(Process->CommitCharge == 0);

    /* Remove us from the list */
    OldIrql = MiAcquireExpansionLock();
    if (Process->Vm.WorkingSetExpansionLinks.Flink != NULL)
        RemoveEntryList(&Process->Vm.WorkingSetExpansionLinks);
    MiReleaseExpansionLock(OldIrql);

    /* Acquire the PFN lock */
    OldIrql = MiAcquirePfnLock();

    /* Check for fully initialized process */
    if (Process->AddressSpaceInitialized == 2)
    {
        /* Map the working set page and its page table */
        Pfn1 = MiGetPfnEntry(Process->WorkingSetPage);
        Pfn2 = MiGetPfnEntry(Pfn1->u4.PteFrame);

        /* Nuke it */
        MI_SET_PFN_DELETED(Pfn1);
        MiDecrementShareCount(Pfn2, Pfn1->u4.PteFrame);
        MiDecrementShareCount(Pfn1, Process->WorkingSetPage);
        ASSERT((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

        /* Now map hyperspace and its page table */
        PageFrameIndex = Process->Pcb.DirectoryTableBase[1] >> PAGE_SHIFT;
        Pfn1 = MiGetPfnEntry(PageFrameIndex);
        Pfn2 = MiGetPfnEntry(Pfn1->u4.PteFrame);

        /* Nuke it */
        MI_SET_PFN_DELETED(Pfn1);
        MiDecrementShareCount(Pfn2, Pfn1->u4.PteFrame);
        MiDecrementShareCount(Pfn1, PageFrameIndex);
        ASSERT((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));

        /* Finally, nuke the PDE itself */
        PageFrameIndex = Process->Pcb.DirectoryTableBase[0] >> PAGE_SHIFT;
        Pfn1 = MiGetPfnEntry(PageFrameIndex);
        MI_SET_PFN_DELETED(Pfn1);
        MiDecrementShareCount(Pfn1, PageFrameIndex);
        MiDecrementShareCount(Pfn1, PageFrameIndex);

        /* Page table is now dead. Bye bye... */
        ASSERT((Pfn1->u3.e2.ReferenceCount == 0) || (Pfn1->u3.e1.WriteInProgress));
    }
    else
    {
        DPRINT1("Deleting partially initialized address space of Process %p. Might leak resources.\n",
                Process);
    }

    /* Release the PFN lock */
    MiReleasePfnLock(OldIrql);

    /* Drop a reference on the session */
    if (Process->Session) MiReleaseProcessReferenceToSessionDataPage(Process->Session);

    /* Clear out the PDE pages */
    Process->Pcb.DirectoryTableBase[0] = 0;
    Process->Pcb.DirectoryTableBase[1] = 0;
}


/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtAllocateUserPhysicalPages(IN HANDLE ProcessHandle,
                            IN OUT PULONG_PTR NumberOfPages,
                            IN OUT PULONG_PTR UserPfnArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtMapUserPhysicalPages(IN PVOID VirtualAddresses,
                       IN ULONG_PTR NumberOfPages,
                       IN OUT PULONG_PTR UserPfnArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtMapUserPhysicalPagesScatter(IN PVOID *VirtualAddresses,
                              IN ULONG_PTR NumberOfPages,
                              IN OUT PULONG_PTR UserPfnArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtFreeUserPhysicalPages(IN HANDLE ProcessHandle,
                        IN OUT PULONG_PTR NumberOfPages,
                        IN OUT PULONG_PTR UserPfnArray)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
