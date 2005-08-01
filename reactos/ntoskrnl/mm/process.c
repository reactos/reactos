/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/process.c
 * PURPOSE:         Memory functions related to Processes
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern ULONG NtMajorVersion;
extern ULONG NtMinorVersion;
extern ULONG NtOSCSDVersion;
extern ULONG NtGlobalFlag;

/* FUNCTIONS *****************************************************************/

PVOID
STDCALL
MiCreatePebOrTeb(PEPROCESS Process,
                 PVOID BaseAddress)
{
    NTSTATUS Status;
    PMADDRESS_SPACE ProcessAddressSpace = &Process->AddressSpace;
    PMEMORY_AREA MemoryArea;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    PVOID AllocatedBase = BaseAddress;
    BoundaryAddressMultiple.QuadPart = 0;

    /* Acquire the Lock */
    MmLockAddressSpace(ProcessAddressSpace);

    /*
     * Create a Peb or Teb.
     * Loop until it works, decreasing by PAGE_SIZE each time. The logic here
     * is that a PEB allocation should never fail since the address is free,
     * while TEB allocation can fail, and we should simply try the address
     * below. Is there a nicer way of doing this automagically? (ie: findning)
     * a gap region? -- Alex
     */
    do {
        DPRINT("Trying to allocate: %x\n", AllocatedBase);
        Status = MmCreateMemoryArea(Process,
                                    ProcessAddressSpace,
                                    MEMORY_AREA_PEB_OR_TEB,
                                    &AllocatedBase,
                                    PAGE_SIZE,
                                    PAGE_READWRITE,
                                    &MemoryArea,
                                    TRUE,
                                    FALSE,
                                    BoundaryAddressMultiple);
        AllocatedBase = RVA(AllocatedBase, -PAGE_SIZE);
    } while (Status != STATUS_SUCCESS);

    /* Initialize the Region */
    MmInitialiseRegion(&MemoryArea->Data.VirtualMemoryData.RegionListHead,
                       PAGE_SIZE,
                       MEM_COMMIT,
                       PAGE_READWRITE);

    /* Reserve the pages */
    MmReserveSwapPages(PAGE_SIZE);

    /* Unlock Address Space */
    DPRINT("Returning\n");
    MmUnlockAddressSpace(ProcessAddressSpace);
    return RVA(AllocatedBase, PAGE_SIZE);
}

VOID
MiFreeStackPage(PVOID Context,
                MEMORY_AREA* MemoryArea,
                PVOID Address,
                PFN_TYPE Page,
                SWAPENTRY SwapEntry,
                BOOLEAN Dirty)
{
    ASSERT(SwapEntry == 0);
    if (Page) MmReleasePageMemoryConsumer(MC_NPPOOL, Page);
}

VOID
STDCALL
MmDeleteKernelStack(PVOID Stack,
                    BOOLEAN GuiStack)
{
    /* Lock the Address Space */
    MmLockAddressSpace(MmGetKernelAddressSpace());

    /* Delete the Stack */
    MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                          Stack,
                          MiFreeStackPage,
                          NULL);

    /* Unlock the Address Space */
    MmUnlockAddressSpace(MmGetKernelAddressSpace());
}

VOID
STDCALL
MmDeleteTeb(PEPROCESS Process,
            PTEB Teb)
{
    PMADDRESS_SPACE ProcessAddressSpace = &Process->AddressSpace;
    PMEMORY_AREA MemoryArea;

    /* Lock the Address Space */
    MmLockAddressSpace(ProcessAddressSpace);
    
    MemoryArea = MmLocateMemoryAreaByAddress(ProcessAddressSpace, (PVOID)Teb);
    if (MemoryArea)
    {
       /* Delete the Teb */
       MmFreeVirtualMemory(Process, MemoryArea);
    }

    /* Unlock the Address Space */
    MmUnlockAddressSpace(ProcessAddressSpace);
}

PVOID
STDCALL
MmCreateKernelStack(BOOLEAN GuiStack)
{
    PMEMORY_AREA StackArea;
    ULONG i;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    PFN_TYPE Page[MM_STACK_SIZE / PAGE_SIZE];
    PVOID KernelStack = NULL;
    NTSTATUS Status;

    /* Initialize the Boundary Address */
    BoundaryAddressMultiple.QuadPart = 0;

    /* Lock the Kernel Address Space */
    MmLockAddressSpace(MmGetKernelAddressSpace());

    /* Create a MAREA for the Kernel Stack */
    Status = MmCreateMemoryArea(NULL,
                                MmGetKernelAddressSpace(),
                                MEMORY_AREA_KERNEL_STACK,
                                &KernelStack,
                                MM_STACK_SIZE,
                                0,
                                &StackArea,
                                FALSE,
                                FALSE,
                                BoundaryAddressMultiple);

    /* Unlock the Address Space */
    MmUnlockAddressSpace(MmGetKernelAddressSpace());

    /* Check for Success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create thread stack\n");
        KEBUGCHECK(0);
    }

    /* Mark the Stack in use */
    for (i = 0; i < (MM_STACK_SIZE / PAGE_SIZE); i++)
    {
        Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page[i]);
    }

    /* Create a Virtual Mapping for it */
    Status = MmCreateVirtualMapping(NULL,
                                    KernelStack,
                                    PAGE_READWRITE,
                                    Page,
                                    MM_STACK_SIZE / PAGE_SIZE);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create Virtual Mapping for Kernel Stack\n");
        KEBUGCHECK(0);
    }

    return KernelStack;
}

NTSTATUS
STDCALL
MmCreatePeb(PEPROCESS Process)
{
    PPEB Peb = NULL;
    LARGE_INTEGER SectionOffset;
    ULONG ViewSize = 0;
    PVOID TableBase = NULL;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    NTSTATUS Status;
    KAFFINITY ProcessAffinityMask = 0;
    SectionOffset.QuadPart = (ULONGLONG)0;

    DPRINT("MmCreatePeb\n");

    /* Map NLS Tables */
    DPRINT("Mapping NLS\n");
    Status = MmMapViewOfSection(NlsSectionObject,
                                Process,
                                &TableBase,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                MEM_TOP_DOWN,
                                PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmMapViewOfSection() failed (Status %lx)\n", Status);
        return(Status);
    }
    DPRINT("TableBase %p  ViewSize %lx\n", TableBase, ViewSize);

    /* Attach to Process */
    KeAttachProcess(&Process->Pcb);

    /* Allocate the PEB */
    Peb = MiCreatePebOrTeb(Process, (PVOID)PEB_BASE);

    /* Initialize the PEB */
    DPRINT("Allocated: %x\n", Peb);
    RtlZeroMemory(Peb, sizeof(PEB));

    /* Set up data */
    DPRINT("Setting up PEB\n");
    Peb->ImageBaseAddress = Process->SectionBaseAddress;
    Peb->InheritedAddressSpace = 0;
    Peb->Mutant = NULL;

    /* NLS */
    Peb->AnsiCodePageData = (char*)TableBase + NlsAnsiTableOffset;
    Peb->OemCodePageData = (char*)TableBase + NlsOemTableOffset;
    Peb->UnicodeCaseTableData = (char*)TableBase + NlsUnicodeTableOffset;

    /* Default Version Data (could get changed below) */
    Peb->OSMajorVersion = NtMajorVersion;
    Peb->OSMinorVersion = NtMinorVersion;
    Peb->OSBuildNumber = 2195;
    Peb->OSPlatformId = 2; /* VER_PLATFORM_WIN32_NT */
    Peb->OSCSDVersion = NtOSCSDVersion;

    /* Heap and Debug Data */
    Peb->NumberOfProcessors = KeNumberProcessors;
    Peb->BeingDebugged = (BOOLEAN)(Process->DebugPort != NULL ? TRUE : FALSE);
    Peb->NtGlobalFlag = NtGlobalFlag;
    /*Peb->HeapSegmentReserve = MmHeapSegmentReserve;
    Peb->HeapSegmentCommit = MmHeapSegmentCommit;
    Peb->HeapDeCommitTotalFreeThreshold = MmHeapDeCommitTotalFreeThreshold;
    Peb->HeapDeCommitFreeBlockThreshold = MmHeapDeCommitFreeBlockThreshold;*/
    Peb->NumberOfHeaps = 0;
    Peb->MaximumNumberOfHeaps = (PAGE_SIZE - sizeof(PEB)) / sizeof(PVOID);
    Peb->ProcessHeaps = (PVOID*)Peb + 1;

    /* Image Data */
    if ((NtHeaders = RtlImageNtHeader(Peb->ImageBaseAddress)))
    {
        /* Get the Image Config Data too */
        ImageConfigData = RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                                       TRUE,
                                                       IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                       &ViewSize);

        /* Write subsystem data */
        Peb->ImageSubSystem = NtHeaders->OptionalHeader.Subsystem;
        Peb->ImageSubSystemMajorVersion = NtHeaders->OptionalHeader.MajorSubsystemVersion;
        Peb->ImageSubSystemMinorVersion = NtHeaders->OptionalHeader.MinorSubsystemVersion;

        /* Write Version Data */
        if (NtHeaders->OptionalHeader.Win32VersionValue)
        {
            Peb->OSMajorVersion = NtHeaders->OptionalHeader.Win32VersionValue & 0xFF;
            Peb->OSMinorVersion = (NtHeaders->OptionalHeader.Win32VersionValue >> 8) & 0xFF;
            Peb->OSBuildNumber = (NtHeaders->OptionalHeader.Win32VersionValue >> 16) & 0x3FFF;

            /* Lie about the version if requested */
            if (ImageConfigData && ImageConfigData->CSDVersion)
            {
                Peb->OSCSDVersion = ImageConfigData->CSDVersion;
            }

            /* Set the Platform ID */
            Peb->OSPlatformId = (NtHeaders->OptionalHeader.Win32VersionValue >> 30) ^ 2;
        }

        /* Check for affinity override */
        if (ImageConfigData && ImageConfigData->ProcessAffinityMask)
        {
            ProcessAffinityMask = ImageConfigData->ProcessAffinityMask;
        }

        /* Check if the image is not safe for SMP */
        if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)
        {
            /* FIXME: Choose one randomly */
            Peb->ImageProcessAffinityMask = 1;
        }
        else
        {
            /* Use affinity from Image Header */
            Peb->ImageProcessAffinityMask = ProcessAffinityMask;
        }
    }

    /* Misc data */
    Peb->SessionId = Process->Session;
    Process->Peb = Peb;

    /* Detach from the Process */
    KeDetachProcess();

    DPRINT("MmCreatePeb: Peb created at %p\n", Peb);
    return STATUS_SUCCESS;
}

PTEB
STDCALL
MmCreateTeb(PEPROCESS Process,
            PCLIENT_ID ClientId,
            PINITIAL_TEB InitialTeb)
{
    PTEB Teb;
    BOOLEAN Attached = FALSE;

    /* Attach to the process */
    DPRINT("MmCreateTeb\n");
    if (Process != PsGetCurrentProcess())
    {
        /* Attach to Target */
        KeAttachProcess(&Process->Pcb);
        Attached = TRUE;
    }

    /* Allocate the TEB */
    Teb = MiCreatePebOrTeb(Process, (PVOID)TEB_BASE);

    /* Initialize the PEB */
    RtlZeroMemory(Teb, sizeof(TEB));

    /* Set TIB Data */
    Teb->Tib.ExceptionList = (PVOID)0xFFFFFFFF;
    Teb->Tib.Version = 1;
    Teb->Tib.Self = (PNT_TIB)Teb;

    /* Set TEB Data */
    Teb->Cid = *ClientId;
    Teb->RealClientId = *ClientId;
    Teb->ProcessEnvironmentBlock = Process->Peb;
    Teb->CurrentLocale = PsDefaultThreadLocaleId;

    /* Store stack information from InitialTeb */
    if(InitialTeb != NULL)
    {
        Teb->Tib.StackBase = InitialTeb->StackBase;
        Teb->Tib.StackLimit = InitialTeb->StackLimit;
        Teb->DeallocationStack = InitialTeb->AllocatedStackBase;
    }

    /* Return TEB Address */
    DPRINT("Allocated: %x\n", Teb);
    if (Attached) KeDetachProcess();
    return Teb;
}

NTSTATUS
STDCALL
MmCreateProcessAddressSpace(IN PEPROCESS Process,
                            IN PSECTION_OBJECT Section OPTIONAL)
{
    NTSTATUS Status;
    PMADDRESS_SPACE ProcessAddressSpace = &Process->AddressSpace;
    PVOID BaseAddress;
    PMEMORY_AREA MemoryArea;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    ULONG ViewSize = 0;
    PVOID ImageBase = 0;
    BoundaryAddressMultiple.QuadPart = 0;

    /* Initialize the Addresss Space */
    MmInitializeAddressSpace(Process, ProcessAddressSpace);

    /* Acquire the Lock */
    MmLockAddressSpace(ProcessAddressSpace);

    /* Protect the highest 64KB of the process address space */
    BaseAddress = (PVOID)MmUserProbeAddress;
    Status = MmCreateMemoryArea(Process,
                                ProcessAddressSpace,
                                MEMORY_AREA_NO_ACCESS,
                                &BaseAddress,
                                0x10000,
                                PAGE_NOACCESS,
                                &MemoryArea,
                                FALSE,
                                FALSE,
                                BoundaryAddressMultiple);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to protect last 64KB\n");
        goto exit;
     }

    /* Protect the 60KB above the shared user page */
    BaseAddress = (char*)USER_SHARED_DATA + PAGE_SIZE;
    Status = MmCreateMemoryArea(Process,
                                ProcessAddressSpace,
                                MEMORY_AREA_NO_ACCESS,
                                &BaseAddress,
                                0x10000 - PAGE_SIZE,
                                PAGE_NOACCESS,
                                &MemoryArea,
                                FALSE,
                                FALSE,
                                BoundaryAddressMultiple);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to protect the memory above the shared user page\n");
        goto exit;
     }

    /* Create the shared data page */
    BaseAddress = (PVOID)USER_SHARED_DATA;
    Status = MmCreateMemoryArea(Process,
                                ProcessAddressSpace,
                                MEMORY_AREA_SHARED_DATA,
                                &BaseAddress,
                                PAGE_SIZE,
                                PAGE_READONLY,
                                &MemoryArea,
                                FALSE,
                                FALSE,
                                BoundaryAddressMultiple);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create Shared User Data\n");
        goto exit;
     }

    /* Check if there's a Section Object */
    if (Section)
    {
        UNICODE_STRING FileName;
        PWCHAR szSrc;
        PCHAR szDest;
        USHORT lnFName = 0;

        /* Unlock the Address Space */
        DPRINT("Unlocking\n");
        MmUnlockAddressSpace(ProcessAddressSpace);

        DPRINT("Mapping process image. Section: %p, Process: %p, ImageBase: %p\n",
                 Section, Process, &ImageBase);
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
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map process Image\n");
            ObDereferenceObject(Section);
            goto exit;
        }
        ObDereferenceObject(Section);

        /* Save the pointer */
        Process->SectionBaseAddress = ImageBase;

        /* Determine the image file name and save it to EPROCESS */
        DPRINT("Getting Image name\n");
        FileName = Section->FileObject->FileName;
        szSrc = (PWCHAR)(FileName.Buffer + (FileName.Length / sizeof(WCHAR)) - 1);

        while(szSrc >= FileName.Buffer)
        {
            if(*szSrc == L'\\')
            {
                szSrc++;
                break;
            }
            else
            {
                szSrc--;
                lnFName++;
            }
        }

        /* Copy the to the process and truncate it to 15 characters if necessary */
        DPRINT("Copying and truncating\n");
        szDest = Process->ImageFileName;
        lnFName = min(lnFName, sizeof(Process->ImageFileName) - 1);
        while(lnFName-- > 0) *(szDest++) = (UCHAR)*(szSrc++);

        /* Return status to caller */
        return Status;
    }

exit:
    /* Unlock the Address Space */
    DPRINT("Unlocking\n");
    MmUnlockAddressSpace(ProcessAddressSpace);

    /* Return status to caller */
    return Status;
}
