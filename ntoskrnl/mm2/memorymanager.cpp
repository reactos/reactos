/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/memorymanager.cpp
 * PURPOSE:         Memory Manager class implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "memorymanager.hpp"

//#define NDEBUG
#include <debug.h>

extern "C"
int /*CDECL*/ atexit(void (*func)(void))
{
    // We just ignore this because the kernel never quits...
    KeBugCheck(0);
    return 0;
}

extern "C" {
void __cxa_pure_virtual()
{
    // put error handling here
    DbgBreakPoint();
    KeBugCheck(0);
}
}

MEMORY_MANAGER::
MEMORY_MANAGER()
{
    DPRINT1("Constructor called!\n");
}

MEMORY_MANAGER::
~MEMORY_MANAGER()
{
}

BOOLEAN
MEMORY_MANAGER::
Initialize0(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //NTSTATUS Status;
#define MAX_MEMORY_RUNS 40

    // TODO: Check struct/POD classes sizes

    // Count physical pages on the system
    HighestPhysicalPage = 0;
    LowestPhysicalPage = (ULONG)-1;
    ScanMemoryDescriptors(LoaderBlock);

    // Zero init various counters
    SystemPageColor = 0;
    MaxWorkingSetSize = 0;
    MinWorkingSetSize = 20;

    OverCommit = 0;
    SystemPtes.NonPagedSystemStart = 0;

    // Define the basic user vs. kernel address space separation
    MmSystemRangeStart = (PVOID)MI_DEFAULT_SYSTEM_RANGE_START;
    MmUserProbeAddress = (ULONG_PTR)MI_USER_PROBE_ADDRESS;
    MmHighestUserAddress = (PVOID)MI_HIGHEST_USER_ADDRESS;

    // Min amount of pages to trigger working set trimming
    MinimumFreePages = 26;

    // Initialize essential lists
    InitializeListHead(&WorkingSetExpansionHead);
    InitializeListHead(&InPageSupportListHead);
    InitializeListHead(&EventCountListHead);

    // Initialize sync primitives
    KeInitializeMutant(&SystemLoadLock, FALSE);
    KeInitializeEvent(&AvailablePagesEvent, NotificationEvent, TRUE);
    KeInitializeEvent(&AvailablePagesEventHigh, NotificationEvent, TRUE);
    KeInitializeEvent(&CollidedFlushEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&CollidedLockEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&MappedFileIoComplete, NotificationEvent, FALSE);
    KeInitializeEvent(&ImageMappingPteEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&ZeroingPageEvent, SynchronizationEvent, FALSE);
    ExInitializeFastMutex(&SectionCommitMutex);
    ExInitializeFastMutex(&SectionBasedMutex);
    ExInitializeResource(&SystemWsLock);
    KeInitializeSpinLock(&ChargeCommitmentLock);
    KeInitializeSpinLock(&ExpansionLock);
    ExInitializeFastMutex(&PageFileCreationLock);
    ExInitializeResource(&SectionExtendResource);
    ExInitializeResource(&SectionExtendSetResource);
    ZeroingPageThreadActive = FALSE;

    // Go through memory
    ULONG MemoryAlloc[(sizeof(PHYSICAL_MEMORY_DESCRIPTOR) + sizeof(PHYSICAL_MEMORY_RUN)*MAX_MEMORY_RUNS) / sizeof(ULONG)];
    ULONG i;

    PPHYSICAL_MEMORY_DESCRIPTOR Memory = (PPHYSICAL_MEMORY_DESCRIPTOR)&MemoryAlloc;
    Memory->NumberOfRuns = MAX_MEMORY_RUNS;

    // Instantiate memory that we don't consider RAM/usable
    // We use the same exclusions that Windows does, in order to try to be
    // compatible with WinLDR-style booting
    BOOLEAN IncludeType[LoaderMaximum];
    for (i = 0; i < LoaderMaximum; i++) IncludeType[i] = TRUE;
    IncludeType[LoaderBad] = FALSE;
    IncludeType[LoaderFirmwarePermanent] = FALSE;
    IncludeType[LoaderSpecialMemory] = FALSE;
    IncludeType[LoaderBBTMemory] = FALSE;

    // Initialize memory limits based on information from the bootloader
    InitializeMemoryLimits(LoaderBlock, IncludeType, Memory);

    // Init number of system PTEs
    SystemPtes.EstimateNumber(Memory->NumberOfPages);

    // Perform machine dependent init
    MachineDependentInit0(LoaderBlock);

    // Move contents of physical memory descriptors into non paged pool allocated storage
    ULONG PhysicalMemoryBlockSize = sizeof(PHYSICAL_MEMORY_DESCRIPTOR) + sizeof(PHYSICAL_MEMORY_RUN) * (Memory->NumberOfRuns - 1);
    PhysicalMemoryBlock = (PPHYSICAL_MEMORY_DESCRIPTOR)Pools.Allocate(NonPagedPool, PhysicalMemoryBlockSize);
    ASSERT(PhysicalMemoryBlockSize <= sizeof(Memory));
    RtlCopyMemory(PhysicalMemoryBlock, Memory, PhysicalMemoryBlockSize);

    // Set the initial resident page count
    ResidentAvailablePages = PfnDb.GetAvailablePages() - 32;

    // Initialize system cache
    SystemCache.Initialize();

    // Finetune the page count by removing working set and NP expansion
    ResidentAvailablePages -= SystemCache.GetWsMin();
    ResidentAvailableAtInit = ResidentAvailablePages;
    if (ResidentAvailablePages <= 0)
    {
        /* This should not happen */
        DPRINT1("System cache working set too big\n");
        return FALSE;
    }

    TotalCommitLimit = PfnDb.GetAvailablePages() << 2;

    // Adjust overcommit value
    if ((OverCommit == 0) && (PfnDb.GetAvailablePages() > 1024))
        OverCommit = PfnDb.GetAvailablePages() - 1024;

    MaxWorkingSetSize = PfnDb.GetAvailablePages() - 512;

    if (MaxWorkingSetSize > (WorkingSetLimit - 5))
        MaxWorkingSetSize = WorkingSetLimit - 5;

    // Initialize MPW event
    KeInitializeEvent(&ModifiedPageWriterEvent, NotificationEvent, FALSE);

    // Time to build paged pool
    BuildPagedPool();

    // If more than 128Mb of memory, then we can add more system PTEs
    if (MmNumberOfPhysicalPages > ((128 * _1MB) >> PAGE_SHIFT))
    {
        DPRINT1("FIXME: Amount of system PTEs could be increased because there is enough RAM available\n");
        //SystemPtes.Grow();
    }

    return TRUE;
}

BOOLEAN
MEMORY_MANAGER::
Initialize1(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    // Perform machine dependent init, phase 1
    MachineDependentInit1(LoaderBlock);

    UNIMPLEMENTED;
    return TRUE;
}

BOOLEAN
MEMORY_MANAGER::
Initialize2(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNIMPLEMENTED;
    return TRUE;
}

VOID
MEMORY_MANAGER::
InitializeMemoryLimits(PLOADER_PARAMETER_BLOCK LoaderBlock, PBOOLEAN IncludeType, PPHYSICAL_MEMORY_DESCRIPTOR Memory)
{
    // Preinitialize output
    Memory->Run[0].BasePage = 0xffffffff;
    Memory->Run[0].PageCount = 0;

    ULONG NumberOfRuns = 0;
    ULONG NumberOfPages = 0;
    ULONG PrevPage = 0;
    ULONG NextPage = 0xffffffff;

    BOOLEAN Loop;
    do
    {
        BOOLEAN Merged = FALSE;
        Loop = FALSE;

        PLIST_ENTRY NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextMd != &LoaderBlock->MemoryDescriptorListHead)
        {
            PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor = CONTAINING_RECORD(NextMd, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);

            // If this memory is included
            if ((MemoryDescriptor->MemoryType < LoaderMaximum) && IncludeType[MemoryDescriptor->MemoryType] )
            {
                // Merge if possible
                if (MemoryDescriptor->BasePage == NextPage)
                {
                    ASSERT (MemoryDescriptor->PageCount != 0);
                    Memory->Run[NumberOfRuns - 1].PageCount += MemoryDescriptor->PageCount;
                    NextPage += MemoryDescriptor->PageCount;
                    NumberOfPages += MemoryDescriptor->PageCount;
                    Merged = TRUE;
                    Loop = TRUE;
                    break;
                }

                if (MemoryDescriptor->BasePage >= PrevPage)
                {
                    if (Memory->Run[NumberOfRuns].BasePage > MemoryDescriptor->BasePage)
                    {
                        Memory->Run[NumberOfRuns].BasePage = MemoryDescriptor->BasePage;
                        Memory->Run[NumberOfRuns].PageCount = MemoryDescriptor->PageCount;
                    }
                    // Do one more iteration
                    Loop = TRUE;
                }
            }

            // Continue to the next memory descriptor
            NextMd = MemoryDescriptor->ListEntry.Flink;
        }

        if (!Merged && Loop)
        {
            NextPage = Memory->Run[NumberOfRuns].BasePage + Memory->Run[NumberOfRuns].PageCount;
            NumberOfPages += Memory->Run[NumberOfRuns].PageCount;
            NumberOfRuns++;
        }
        Memory->Run[NumberOfRuns].BasePage = 0xffffffff;
        PrevPage = NextPage;
    } while (Loop);

    // We should not have been able to go past our initial estimate
    ASSERT(NumberOfRuns <= Memory->NumberOfRuns);

    // Write the final numbers, and return it
    Memory->NumberOfRuns = NumberOfRuns;
    Memory->NumberOfPages = NumberOfPages;
}

VOID
MEMORY_MANAGER::
ScanMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY ListEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PFN_NUMBER PageFrameIndex, FreePages = 0;

    LbNumberDescriptors = 0;
    LbNumberOfFreePages = 0;

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);
        DPRINT("MD Type: %lx Base: %lx Count: %lx\n",
            Descriptor->MemoryType, Descriptor->BasePage, Descriptor->PageCount);

        /* Count this descriptor */
        LbNumberDescriptors++;

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
            MmNumberOfPhysicalPages += (PFN_COUNT)Descriptor->PageCount;
        }

        /* Check if this is the new lowest page */
        if (Descriptor->BasePage < LowestPhysicalPage)
        {
            /* Update the lowest page */
            LowestPhysicalPage = Descriptor->BasePage;
        }

        /* Check if this is the new highest page */
        PageFrameIndex = Descriptor->BasePage + Descriptor->PageCount;
        if (PageFrameIndex > HighestPhysicalPage)
        {
            /* Update the highest page */
            HighestPhysicalPage = PageFrameIndex - 1;
        }

        /* Check if this is free memory */
        if ((Descriptor->MemoryType == LoaderFree) ||
            (Descriptor->MemoryType == LoaderLoadedProgram) ||
            (Descriptor->MemoryType == LoaderFirmwareTemporary) ||
            (Descriptor->MemoryType == LoaderOsloaderStack))
        {
            /* Count it too free pages */
            LbNumberOfFreePages += Descriptor->PageCount;

            /* Check if this is the largest memory descriptor */
            if (Descriptor->PageCount > FreePages)
            {
                /* Remember it */
                LbFreeDescriptor = Descriptor;
                FreePages = Descriptor->PageCount;
            }
        }
    }

    /* Save original values of the free descriptor, since it'll be
     * altered by early allocations */
    LbOldFreeDescriptor = *LbFreeDescriptor;
}

PFN_NUMBER
MEMORY_MANAGER::
LbGetNextPage(PFN_NUMBER PageCount)
{
    PFN_NUMBER Pfn;

    /* Make sure we have enough pages */
    if (PageCount > LbFreeDescriptor->PageCount)
    {
        /* Crash the system */
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     LbFreeDescriptor->PageCount,
                     LbOldFreeDescriptor.PageCount,
                     PageCount);
    }

    // Use our lowest usable free pages
    Pfn = LbFreeDescriptor->BasePage;
    LbFreeDescriptor->BasePage += PageCount;
    LbFreeDescriptor->PageCount -= PageCount;
    return Pfn;
}

VOID
MEMORY_MANAGER::
BuildPagedPool()
{
    UNIMPLEMENTED;
}

VOID
MEMORY_MANAGER::
GrowWsleHash(WORKING_SET *Ws, BOOLEAN PfnLockAcquired)
{
    UNIMPLEMENTED;
}

VOID
MEMORY_MANAGER::
ObtainFreePages()
{
    UNIMPLEMENTED;
}

VOID
MEMORY_MANAGER::
RestoreTransitionPte(ULONG PageFrameIndex)
{
    UNIMPLEMENTED;
}

NTSTATUS
MEMORY_MANAGER::
HandleAccessFault(IN BOOLEAN StoreInstruction,
                  IN PVOID Address,
                  IN KPROCESSOR_MODE Mode,
                  IN PVOID TrapInformation)
{
    BOOLEAN ContinueProcessing;
    NTSTATUS Status;

    // Get current process and IRQL
    //PEPROCESS Process = PsGetCurrentProcess();
    KIRQL Irql = KeGetCurrentIrql();

    DPRINT("Access Fault %p\n", Address);
    DbgBreakPoint();
    // Get PTE and PDE of the address in question
    //PTENTRY *Pte = PTENTRY::AddressToPte(Address);
    //PTENTRY *Pde = PTENTRY::AddressToPde(Address);

    if (Irql > APC_LEVEL)
    {
        UNIMPLEMENTED;
    }

    if (Address >= MM_SYSTEM_RANGE_START)
    {
        // Fault in the system address space
        Status = HandleSystemAddressFault(StoreInstruction, (ULONG_PTR)Address, Mode, TrapInformation, &ContinueProcessing);
        if (!ContinueProcessing) return Status;
    }

    // FIXME: Check if modified list is too big

    // Disable APCs and lock working set
    KeRaiseIrql(APC_LEVEL, &Irql);

    UNIMPLEMENTED;

    KeLowerIrql(Irql);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MEMORY_MANAGER::
HandleSystemAddressFault(IN BOOLEAN StoreInstruction,
                         IN ULONG_PTR Address,
                         IN KPROCESSOR_MODE Mode,
                         IN PVOID TrapInformation,
                         IN BOOLEAN *Continue)
{
    // Default action is to stop further processing the page fault
    *Continue = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    // Prohibit access from user mode
    if (Mode == UserMode) return STATUS_ACCESS_VIOLATION;

    // Get PTE and PDE of the address in question
    PTENTRY *Pte = PTENTRY::AddressToPte(Address);
    PTENTRY *Pde = PTENTRY::AddressToPde(Address);

    // Check if PDE is valid
    if (Pde->IsValid())
    {
        // Large page fault - ignore
        if (Pde->IsLargePage()) return STATUS_SUCCESS;

        // Check PTE
        if (Pte->IsValid())
        {
            UNIMPLEMENTED;
            return STATUS_SUCCESS;
        }
    }
    else
    {
        // Check and lazily initialize this PDE
        Status = CheckAndInitPde(Address);

        // If it's still invalid - that's a huge problem
        if (!Pde->IsValid())
        {
            KeBugCheckEx(PAGE_FAULT_IN_NONPAGED_AREA,
                         Address,
                         StoreInstruction,
                         Mode,
                         2);
        }
        // FIXME: Check the PTE now
        UNIMPLEMENTED;
    }

    if ((Address < PTE_BASE) || (Address > HYPER_SPACE_END))
    {
        UNIMPLEMENTED;
    }
    else
    {
        // Check and lazily init our PDE, and check for magic code
        if (CheckAndInitPde(Address) == STATUS_WAIT_1)
        {
            // It means this was paged pool's PTE
            return STATUS_SUCCESS;
        }
    }

    *Continue = TRUE;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
MEMORY_MANAGER::
CheckAndInitPde(ULONG_PTR Address)
{
    PTENTRY *SystemRangePte = PTENTRY::AddressToPte((ULONG_PTR)MM_SYSTEM_RANGE_START);
    PTENTRY *Pde = PTENTRY::AddressToPde(Address);
    NTSTATUS Status = STATUS_SUCCESS;

    if ((Address >= (ULONG_PTR)SystemRangePte) && (Address <= PDE_TOP))
    {
        // This is a PTE belonging to paged pool
        Status = STATUS_WAIT_1;
    }
    else if (Address < (ULONG_PTR)MM_SYSTEM_RANGE_START)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    // Initialize this PDE
    if (!Pde->IsValid())
    {
        // Get address of PTE
        //PTENTRY *Pte = (PTENTRY *)Pde->PteToAddress();

        UNIMPLEMENTED;
    }

    return Status;
}

VOID
MEMORY_MANAGER::
WakeupZeroingThread()
{
    // Don't wake up if it's already active
    if (ZeroingPageThreadActive) return;

    // Mark it as active and signal its event
    ZeroingPageThreadActive = TRUE;
    KeSetEvent(&ZeroingPageEvent, 0, FALSE);
}

VOID
MEMORY_MANAGER::
WakeupMPWThread()
{
    KeSetEvent(&ModifiedPageWriterEvent, 0, FALSE);
}

// WORK IN PROGRESS
NTSTATUS
MEMORY_MANAGER::
InitializeProcessAddressSpace(PEPROCESS Process, PEPROCESS ProcessClone, PVOID Section, PULONG Flags, POBJECT_NAME_INFORMATION *AuditName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    //SIZE_T ViewSize = 0;
    //PVOID ImageBase = 0;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameNumber;
    //UNICODE_STRING FileName;
    //PWCHAR Source;
    //PCHAR Destination;
    //USHORT Length = 0;
    PTENTRY TempPte;

    /* We should have a PDE */
    //ASSERT(Process->Pcb.DirectoryTableBase[0] != 0);
    //ASSERT(Process->PdeUpdateNeeded == FALSE);

    /* Attach to the process */
    KeAttachProcess(&Process->Pcb);

    /* The address space should now been in phase 1 or 0 */
    ASSERT(Process->AddressSpaceInitialized <= 1);
    Process->AddressSpaceInitialized = 2;

    /* Initialize the Addresss Space lock */
    KeInitializeGuardedMutex(&Process->AddressCreationLock);
    KeInitializeGuardedMutex(&Process->WorkingSetLock);

    // Set LastTrimTime and working set list in the process
    GET_WS_OBJECT(Process)->UpdateLastTrimTime();
    GET_WS_OBJECT(Process)->SetWorkingListSet(WorkingSetList);

    /* Lock PFN database */
    OldIrql = PfnDb.AcquireLock();

    /* Setup the PFN for the PDE base of this process */
    PTENTRY *PointerPte = PTENTRY::AddressToPte(PDE_BASE);
    PageFrameNumber = PointerPte->GetPfn();
    ASSERT(Process->Pcb.DirectoryTableBase[0] == PageFrameNumber * PAGE_SIZE);
    PfnDb.InitializeElement(PageFrameNumber, PointerPte, TRUE);

    /* Do the same for hyperspace */
    PointerPte = PTENTRY::AddressToPde(HYPER_SPACE);
    PageFrameNumber = PointerPte->GetPfn();
    PfnDb.InitializeElement(PageFrameNumber, PointerPte, TRUE);

    /* Release PFN lock */
    PfnDb.ReleaseLock(OldIrql);

    /* Setup the PFN for the PTE for the working set */
    PointerPte = PTENTRY::AddressToPte((ULONG_PTR)WorkingSetList);
    PointerPte->u.Long = DemandZeroPte.u.Long;
    PfnDb.InitializeElement(Process->WorkingSetPage, PointerPte, TRUE);

    TempPte.MakeValidPte(Process->WorkingSetPage, MM_READWRITE, PointerPte);
    TempPte.SetDirty();

    *PointerPte = TempPte;

    /* Now initialize the working set list */
    WorkingSetList->Initialize(Process);

    /* Sanity check */
    ASSERT(Process->PhysicalVadRoot == NULL);

    /* Check if there's a Section Object */
    if (Section)
    {
#if 0
        /* Determine the image file name and save it to EPROCESS */
        FileName = SectionObject->FileObject->FileName;
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
            Status = SeInitializeProcessAuditName(SectionObject->FileObject,
                                                  FALSE,
                                                  AuditName);
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
                                    0,
                                    MEM_COMMIT,
                                    PAGE_READWRITE);

        /* Save the pointer */
        Process->SectionBaseAddress = ImageBase;
#else
        UNIMPLEMENTED;
#endif
    }

    if (ProcessClone)
    {
        UNIMPLEMENTED;
    }

    /* Be nice and detach */
    KeDetachProcess();

    /* Return status to caller */
    return Status;
}
