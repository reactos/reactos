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

#line 15 "ARM³::PROCSUP"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

extern MM_SYSTEMSIZE MmSystemSize;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiRosTakeOverPebTebRanges(IN PEPROCESS Process)
{
    NTSTATUS Status;
    PMEMORY_AREA MemoryArea;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    PVOID AllocatedBase = (PVOID)MI_LOWEST_VAD_ADDRESS;
    BoundaryAddressMultiple.QuadPart = 0;

    Status = MmCreateMemoryArea(&Process->Vm,
                                MEMORY_AREA_OWNED_BY_ARM3,
                                &AllocatedBase,
                                ((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS - 1) -
                                (ULONG_PTR)MI_LOWEST_VAD_ADDRESS,
                                PAGE_READWRITE,
                                &MemoryArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(NT_SUCCESS(Status));
}

NTSTATUS
NTAPI
MiCreatePebOrTeb(IN PEPROCESS Process,
                 IN ULONG Size,
                 OUT PULONG_PTR Base)
{
    PETHREAD Thread = PsGetCurrentThread();
    PMMVAD_LONG Vad;
    NTSTATUS Status;
    ULONG RandomCoeff;
    ULONG_PTR StartAddress, EndAddress;
    LARGE_INTEGER CurrentTime;
    
    /* Allocate a VAD */
    Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD_LONG), 'ldaV');
    if (!Vad) return STATUS_NO_MEMORY;
    
    /* Setup the primary flags with the size, and make it commited, private, RW */
    Vad->u.LongFlags = 0;
    Vad->u.VadFlags.CommitCharge = BYTES_TO_PAGES(Size);
    Vad->u.VadFlags.MemCommit = TRUE;
    Vad->u.VadFlags.PrivateMemory = TRUE;
    Vad->u.VadFlags.Protection = MM_READWRITE;
    Vad->u.VadFlags.NoChange = TRUE;
    
    /* Setup the secondary flags to make it a secured, writable, long VAD */
    Vad->u2.LongFlags2 = 0;
    Vad->u2.VadFlags2.OneSecured = TRUE;
    Vad->u2.VadFlags2.LongVad = TRUE;
    Vad->u2.VadFlags2.ReadOnly = FALSE;
    
    /* Lock the process address space */
    KeAcquireGuardedMutex(&Process->AddressCreationLock);

    /* Check if this is a PEB creation */
    if (Size == sizeof(PEB))
    {
        /* Start at the highest valid address */
        StartAddress = (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1;

        /* Select the random coefficient */
        KeQueryTickCount(&CurrentTime);
        CurrentTime.LowPart &= ((64 * _1KB) >> PAGE_SHIFT) - 1;
        if (CurrentTime.LowPart <= 1) CurrentTime.LowPart = 2;
        RandomCoeff = CurrentTime.LowPart << PAGE_SHIFT;

        /* Select the highest valid address minus the random coefficient */
        StartAddress -= RandomCoeff;
        EndAddress = StartAddress + ROUND_TO_PAGES(Size) - 1;

        /* See if this VA range can be obtained */
        if (!MiCheckForConflictingNode(StartAddress >> PAGE_SHIFT,
                                       EndAddress >> PAGE_SHIFT,
                                       &Process->VadRoot))
        {
            /* No conflict, use this address */
            *Base = StartAddress;
            goto AfterFound;
        }
    }
    
    /* For TEBs, or if a PEB location couldn't be found, scan the VAD root */
    Status = MiFindEmptyAddressRangeDownTree(ROUND_TO_PAGES(Size),
                                             (ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1,
                                             PAGE_SIZE,
                                             &Process->VadRoot,
                                             Base);
    ASSERT(NT_SUCCESS(Status));
    
AfterFound:
    /* Validate that it came from the VAD ranges */
    ASSERT(*Base >= (ULONG_PTR)MI_LOWEST_VAD_ADDRESS);
    
    /* Build the rest of the VAD now */
    Vad->StartingVpn = (*Base) >> PAGE_SHIFT;
    Vad->EndingVpn =  ((*Base) + Size - 1) >> PAGE_SHIFT;
    Vad->u3.Secured.StartVpn = *Base;
    Vad->u3.Secured.EndVpn = (Vad->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1);
    
    /* FIXME: Should setup VAD bitmap */
    Status = STATUS_SUCCESS;

    /* Pretend as if we own the working set */
    MiLockProcessWorkingSet(Process, Thread);
    
    /* Insert the VAD */
    ASSERT(Vad->EndingVpn >= Vad->StartingVpn);
    Process->VadRoot.NodeHint = Vad;
    MiInsertNode((PVOID)Vad, &Process->VadRoot);
    
    /* Release the working set */
    MiUnlockProcessWorkingSet(Process, Thread);

    /* Release the address space lock */
    KeReleaseGuardedMutex(&Process->AddressCreationLock);

    /* Return the status */
    DPRINT("Allocated PEB/TEB at: 0x%p for %16s\n", *Base, Process->ImageFileName);
    return Status;
}

VOID
NTAPI
MmDeleteTeb(IN PEPROCESS Process,
            IN PTEB Teb)
{
    /* Oops J */
    DPRINT("Leaking 4KB at thread exit, this will be fixed later\n");
}

VOID
NTAPI
MmDeleteKernelStack(IN PVOID StackBase,
                    IN BOOLEAN GuiStack)
{
    PMMPTE PointerPte;
    PFN_NUMBER StackPages, PageFrameNumber;//, PageTableFrameNumber;
    PMMPFN Pfn1;//, Pfn2;
    ULONG i;
    KIRQL OldIrql;
    
    //
    // This should be the guard page, so decrement by one
    //
    PointerPte = MiAddressToPte(StackBase);
    PointerPte--;
    
    //
    // Calculate pages used
    //
    StackPages = BYTES_TO_PAGES(GuiStack ?
                                KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE);
    
    /* Acquire the PFN lock */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
                            
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
#if 0 // ARM3 might not own the page table, so don't take this risk. Leak it instead!
            /* Now get the page of the page table mapping it */
            PageTableFrameNumber = Pfn1->u4.PteFrame;
            Pfn2 = MiGetPfnEntry(PageTableFrameNumber);
            
            /* Remove a shared reference, since the page is going away */
            MiDecrementShareCount(Pfn2, PageTableFrameNumber);
#endif
            /* Set the special pending delete marker */
            Pfn1->PteAddress = (PMMPTE)((ULONG_PTR)Pfn1->PteAddress | 1);
            
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
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

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
    PFN_NUMBER StackPtes, StackPages;
    PMMPTE PointerPte, StackPte;
    PVOID BaseAddress;
    MMPTE TempPte, InvalidPte;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    ULONG i;
    
    //
    // Calculate pages needed
    //
    if (GuiStack)
    {
        //
        // We'll allocate 64KB stack, but only commit 12K
        //
        StackPtes = BYTES_TO_PAGES(KERNEL_LARGE_STACK_SIZE);
        StackPages = BYTES_TO_PAGES(KERNEL_LARGE_STACK_COMMIT);
        
    }
    else
    {
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
    if (GuiStack) PointerPte += BYTES_TO_PAGES(KERNEL_LARGE_STACK_SIZE -
                                               KERNEL_LARGE_STACK_COMMIT);
    

    /* Setup the temporary invalid PTE */
    MI_MAKE_SOFTWARE_PTE(&InvalidPte, MM_NOACCESS);

    /* Setup the template stack PTE */
    MI_MAKE_HARDWARE_PTE_KERNEL(&TempPte, PointerPte + 1, MM_READWRITE, 0);
    
    //
    // Acquire the PFN DB lock
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    
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
        PageFrameIndex = MiRemoveAnyPage(0);
        MI_WRITE_INVALID_PTE(PointerPte, InvalidPte);

        /* Initialize the PFN entry for this page */
        MiInitializePfn(PageFrameIndex, PointerPte, 1);
        
        /* Write the valid PTE */
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE(PointerPte, TempPte);
    }

    // Bug #4835
    (VOID)InterlockedExchangeAddUL(&MiMemoryConsumers[MC_NPPOOL].PagesUsed, StackPages);

    //
    // Release the PFN lock
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    
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
    PFN_NUMBER StackPages;
    KIRQL OldIrql;
    MMPTE TempPte, InvalidPte;
    PFN_NUMBER PageFrameIndex;
    
    //
    // Make sure the stack did not overflow
    //
    ASSERT(((ULONG_PTR)Thread->StackBase - (ULONG_PTR)Thread->StackLimit) <=
           (KERNEL_LARGE_STACK_SIZE + PAGE_SIZE));
    
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
                                     KERNEL_LARGE_STACK_SIZE));
    if (NewLimitPte < LastPte)
    {
        //
        // Sorry!
        //
        DPRINT1("Thread wants too much stack\n");
        return STATUS_STACK_OVERFLOW;
    }
    
    //
    // Calculate the number of new pages
    //
    LimitPte--;
    StackPages = (LimitPte - NewLimitPte + 1);
    
    /* Setup the temporary invalid PTE */
    MI_MAKE_SOFTWARE_PTE(&InvalidPte, MM_NOACCESS);
    
    //
    // Acquire the PFN DB lock
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    //
    // Loop each stack page
    //
    while (LimitPte >= NewLimitPte)
    {
        /* Get a page and write the current invalid PTE */
        PageFrameIndex = MiRemoveAnyPage(0);
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
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    
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

LCID
NTAPI
MmGetSessionLocaleId(VOID)
{
    PEPROCESS Process;
    PAGED_CODE();
    
    //
    // Get the current process
    //
    Process = PsGetCurrentProcess();
    
    //
    // Check if it's the Session Leader
    //
    if (Process->Vm.Flags.SessionLeader)
    {
        //
        // Make sure it has a valid Session
        //
        if (Process->Session)
        {
            //
            // Get the Locale ID
            //
#if ROS_HAS_SESSIONS
            return ((PMM_SESSION_SPACE)Process->Session)->LocaleId;
#endif
        }
    }
    
    //
    // Not a session leader, return the default
    //
    return PsDefaultThreadLocaleId;
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
    KAFFINITY ProcessAffinityMask = 0;
    SectionOffset.QuadPart = (ULONGLONG)0;
    *BasePeb = NULL;
    
    //
    // Attach to Process
    //
    KeAttachProcess(&Process->Pcb);
       
    //
    // Allocate the PEB
    //
    Status = MiCreatePebOrTeb(Process, sizeof(PEB), (PULONG_PTR)&Peb);
    ASSERT(NT_SUCCESS(Status));

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
    if (!NT_SUCCESS(Status)) return Status;
    
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
        Peb->OSPlatformId = 2; /* VER_PLATFORM_WIN32_NT */
        Peb->OSCSDVersion = (USHORT)CmNtCSDVersion;
        
        //
        // Heap and Debug Data
        //
        Peb->NumberOfProcessors = KeNumberProcessors;
        Peb->BeingDebugged = (BOOLEAN)(Process->DebugPort != NULL ? TRUE : FALSE);
        Peb->NtGlobalFlag = NtGlobalFlag;
        /*Peb->HeapSegmentReserve = MmHeapSegmentReserve;
         Peb->HeapSegmentCommit = MmHeapSegmentCommit;
         Peb->HeapDeCommitTotalFreeThreshold = MmHeapDeCommitTotalFreeThreshold;
         Peb->HeapDeCommitFreeBlockThreshold = MmHeapDeCommitFreeBlockThreshold;
         Peb->CriticalSectionTimeout = MmCriticalSectionTimeout;
         Peb->MinimumStackCommit = MmMinimumStackCommitInBytes;
         */
        Peb->MaximumNumberOfHeaps = (PAGE_SIZE - sizeof(PEB)) / sizeof(PVOID);
        Peb->ProcessHeaps = (PVOID*)(Peb + 1);
        
        //
        // Session ID
        //
        if (Process->Session) Peb->SessionId = 0; // MmGetSessionId(Process);
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
            }
            
            //
            // Process the image config data overrides if specfied
            //
            if (ImageConfigData != NULL)
            {
                //
                // Process CSD version override
                //
                if (ImageConfigData->CSDVersion)
                {
                    //
                    // Set new data
                    //
                    Peb->OSCSDVersion = ImageConfigData->CSDVersion;
                }
                
                //
                // Process affinity mask ovverride
                //
                if (ImageConfigData->ProcessAffinityMask)
                {
                    //
                    // Set new data
                    //
                    ProcessAffinityMask = ImageConfigData->ProcessAffinityMask;
                }
            }
            
            //
            // Check if this is a UP image
            if (Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)
            {
                //
                // Force it to use CPU 0
                //
                Peb->ImageProcessAffinityMask = 0;
            }
            else
            {
                //
                // Whatever was configured
                //
                Peb->ImageProcessAffinityMask = ProcessAffinityMask;
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
    ASSERT(NT_SUCCESS(Status));
    
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
        Teb->NtTib.ExceptionList = EXCEPTION_CHAIN_END;
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
