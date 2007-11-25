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
extern ULONG CmNtCSDVersion;
extern ULONG NtBuildNumber;
extern MM_SYSTEMSIZE MmSystemSize;

#define MM_HIGHEST_VAD_ADDRESS \
    (PVOID)((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (16 * PAGE_SIZE))

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
MmSetMemoryPriorityProcess(IN PEPROCESS Process,
                           IN UCHAR MemoryPriority)
{
    UCHAR OldPriority;

    /* Check if we have less then 16MB of Physical Memory */
    if ((MmSystemSize == MmSmallSystem) &&
        (MmStats.NrTotalPages < ((15 * 1024 * 1024) / PAGE_SIZE)))
    {
        /* Always use background priority */
        MemoryPriority = 0;
    }

    /* Save the old priority and update it */
    OldPriority = (UCHAR)Process->Vm.Flags.MemoryPriority;
    Process->Vm.Flags.MemoryPriority = MemoryPriority;

    /* Return the old priority */
    return OldPriority;
}

LCID
NTAPI
MmGetSessionLocaleId(VOID)
{
    PEPROCESS Process;
    PAGED_CODE();

    /* Get the current process */
    Process = PsGetCurrentProcess();

    /* Check if it's the Session Leader */
    if (Process->Vm.Flags.SessionLeader)
    {
        /* Make sure it has a valid Session */
        if (Process->Session)
        {
            /* Get the Locale ID */
#if ROS_HAS_SESSIONS
            return ((PMM_SESSION_SPACE)Process->Session)->LocaleId;
#endif
        }
    }

    /* Not a session leader, return the default */
    return PsDefaultThreadLocaleId;
}

PVOID
STDCALL
MiCreatePebOrTeb(PEPROCESS Process,
                 PVOID BaseAddress)
{
    NTSTATUS Status;
    PMADDRESS_SPACE ProcessAddressSpace = (PMADDRESS_SPACE)&Process->VadRoot;
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
        Status = MmCreateMemoryArea(ProcessAddressSpace,
                                    MEMORY_AREA_PEB_OR_TEB,
                                    &AllocatedBase,
                                    PAGE_SIZE,
                                    PAGE_READWRITE,
                                    &MemoryArea,
                                    TRUE,
                                    0,
                                    BoundaryAddressMultiple);
        AllocatedBase = RVA(AllocatedBase, -PAGE_SIZE);
    } while (Status != STATUS_SUCCESS);

    /* Initialize the Region */
    MmInitializeRegion(&MemoryArea->Data.VirtualMemoryData.RegionListHead,
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
    PMADDRESS_SPACE ProcessAddressSpace = (PMADDRESS_SPACE)&Process->VadRoot;
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
MmCreateKernelStack(BOOLEAN GuiStack,
                    UCHAR Node)
{
    PMEMORY_AREA StackArea;
    ULONG i;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    ULONG StackSize = GuiStack ? KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE;
    PFN_TYPE Page[KERNEL_LARGE_STACK_SIZE / PAGE_SIZE];
    PVOID KernelStack = NULL;
    NTSTATUS Status;

    /* Initialize the Boundary Address */
    BoundaryAddressMultiple.QuadPart = 0;

    /* Lock the Kernel Address Space */
    MmLockAddressSpace(MmGetKernelAddressSpace());

    /* Create a MAREA for the Kernel Stack */
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_KERNEL_STACK,
                                &KernelStack,
                                StackSize,
                                PAGE_READWRITE,
                                &StackArea,
                                FALSE,
                                0,
                                BoundaryAddressMultiple);

    /* Unlock the Address Space */
    MmUnlockAddressSpace(MmGetKernelAddressSpace());

    /* Check for Success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create thread stack\n");
        KEBUGCHECK(0);
    }

    /*
     * Mark the Stack in use.
     * Note: Currently we mark all 60KB in use for a GUI Thread.
     * We should only do this inside MmGrowKernelStack. TODO!
     */
    for (i = 0; i < (StackSize / PAGE_SIZE); i++)
    {
        Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page[i]);
    }

    /* Create a Virtual Mapping for it */
    Status = MmCreateVirtualMapping(NULL,
                                    KernelStack,
                                    PAGE_READWRITE,
                                    Page,
                                    StackSize / PAGE_SIZE);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create Virtual Mapping for Kernel Stack\n");
        KEBUGCHECK(0);
    }

    /* Return the stack base */
    return (PVOID)((ULONG_PTR)KernelStack +
                   (GuiStack ? KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE));
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
MmGrowKernelStack(PVOID StackPointer)
{
    PETHREAD Thread = PsGetCurrentThread();

    /* Make sure we have reserved space for our grow */
    ASSERT(((PCHAR)Thread->Tcb.StackBase - (PCHAR)Thread->Tcb.StackLimit) <=
           (KERNEL_LARGE_STACK_SIZE + PAGE_SIZE));

    /*
     * We'll give you three more pages.
     * NOTE: See note in MmCreateKernelStack. These pages are already being reserved.
     * It would be more efficient to only grow them (commit them) here.
     */
    Thread->Tcb.StackLimit -= KERNEL_STACK_SIZE;

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
MmCreatePeb(PEPROCESS Process)
{
    PPEB Peb = NULL;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize = 0;
    PVOID TableBase = NULL;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData;
    NTSTATUS Status;
    KAFFINITY ProcessAffinityMask = 0;
    SectionOffset.QuadPart = (ULONGLONG)0;
    DPRINT("MmCreatePeb\n");

    /* Allocate the PEB */
    Peb = MiCreatePebOrTeb(Process,
                           (PVOID)((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1));
    ASSERT(Peb == (PVOID)0x7FFDF000);

    /* Map NLS Tables */
    DPRINT("Mapping NLS\n");
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
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmMapViewOfSection() failed (Status %lx)\n", Status);
        return(Status);
    }
    DPRINT("TableBase %p  ViewSize %lx\n", TableBase, ViewSize);

    /* Attach to Process */
    KeAttachProcess(&Process->Pcb);

    /* Initialize the PEB */
    DPRINT("Allocated: %x\n", Peb);
    RtlZeroMemory(Peb, sizeof(PEB));

    /* Set up data */
    DPRINT("Setting up PEB\n");
    Peb->ImageBaseAddress = Process->SectionBaseAddress;
    Peb->InheritedAddressSpace = 0;
    Peb->Mutant = NULL;

    /* NLS */
    Peb->AnsiCodePageData = (PCHAR)TableBase + ExpAnsiCodePageDataOffset;
    Peb->OemCodePageData = (PCHAR)TableBase + ExpOemCodePageDataOffset;
    Peb->UnicodeCaseTableData = (PCHAR)TableBase + ExpUnicodeCaseTableDataOffset;

    /* Default Version Data (could get changed below) */
    Peb->OSMajorVersion = NtMajorVersion;
    Peb->OSMinorVersion = NtMinorVersion;
    Peb->OSBuildNumber = (USHORT)(NtBuildNumber & 0x3FFF);
    Peb->OSPlatformId = 2; /* VER_PLATFORM_WIN32_NT */
    Peb->OSCSDVersion = (USHORT)CmNtCSDVersion;

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

            /* Set the Platform ID */
            Peb->OSPlatformId = (NtHeaders->OptionalHeader.Win32VersionValue >> 30) ^ 2;
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

        _SEH_TRY
        {
            /* Get the Image Config Data too */
            ImageConfigData = RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                                           TRUE,
                                                           IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                           &ViewSize);

            ProbeForRead(ImageConfigData,
                         sizeof(IMAGE_LOAD_CONFIG_DIRECTORY),
                         sizeof(ULONG));

            /* Process the image config data overrides if specfied. */
            if (ImageConfigData != NULL)
            {
                if (ImageConfigData->CSDVersion)
                {
                    Peb->OSCSDVersion = ImageConfigData->CSDVersion;
                }
                if (ImageConfigData->ProcessAffinityMask)
                {
                    ProcessAffinityMask = ImageConfigData->ProcessAffinityMask;
                }
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Misc data */
    Peb->SessionId = Process->Session;
    Process->Peb = Peb;

    /* Detach from the Process */
    KeDetachProcess();

    DPRINT("MmCreatePeb: Peb created at %p\n", Peb);
    return Status;
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
    Teb = MiCreatePebOrTeb(Process,
                           (PVOID)((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1));

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

    /* Initialize the static unicode string */
    Teb->StaticUnicodeString.Length = 0;
    Teb->StaticUnicodeString.MaximumLength = sizeof(Teb->StaticUnicodeBuffer);
    Teb->StaticUnicodeString.Buffer = Teb->StaticUnicodeBuffer;

    /* Return TEB Address */
    DPRINT("Allocated: %x\n", Teb);
    if (Attached) KeDetachProcess();
    return Teb;
}

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess2(IN PEPROCESS Process)
{
    PVOID BaseAddress;
    PMEMORY_AREA MemoryArea;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    NTSTATUS Status;
    PMADDRESS_SPACE ProcessAddressSpace = (PMADDRESS_SPACE)&Process->VadRoot;
    BoundaryAddressMultiple.QuadPart = 0;

    /* Create the shared data page */
    BaseAddress = (PVOID)USER_SHARED_DATA;
    Status = MmCreateMemoryArea(ProcessAddressSpace,
                                MEMORY_AREA_SHARED_DATA,
                                &BaseAddress,
                                PAGE_SIZE,
                                PAGE_EXECUTE_READ,
                                &MemoryArea,
                                FALSE,
                                0,
                                BoundaryAddressMultiple);
    return Status;
}

NTSTATUS
NTAPI
MmInitializeProcessAddressSpace(IN PEPROCESS Process,
                                IN PEPROCESS ProcessClone OPTIONAL,
                                IN PVOID Section OPTIONAL,
                                IN OUT PULONG Flags,
                                IN POBJECT_NAME_INFORMATION *AuditName OPTIONAL)
{
    NTSTATUS Status;
    PMADDRESS_SPACE ProcessAddressSpace = (PMADDRESS_SPACE)&Process->VadRoot;
    PVOID BaseAddress;
    PMEMORY_AREA MemoryArea;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    SIZE_T ViewSize = 0;
    PVOID ImageBase = 0;
    PROS_SECTION_OBJECT SectionObject = Section;
    BoundaryAddressMultiple.QuadPart = 0;

    /* Initialize the Addresss Space */
    MmInitializeAddressSpace(Process, ProcessAddressSpace);

    /* Acquire the Lock */
    MmLockAddressSpace(ProcessAddressSpace);

    /* Protect the highest 64KB of the process address space */
    BaseAddress = (PVOID)MmUserProbeAddress;
    Status = MmCreateMemoryArea(ProcessAddressSpace,
                                MEMORY_AREA_NO_ACCESS,
                                &BaseAddress,
                                0x10000,
                                PAGE_NOACCESS,
                                &MemoryArea,
                                FALSE,
                                0,
                                BoundaryAddressMultiple);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to protect last 64KB\n");
        goto exit;
     }

    /* Protect the 60KB above the shared user page */
    BaseAddress = (char*)USER_SHARED_DATA + PAGE_SIZE;
    Status = MmCreateMemoryArea(ProcessAddressSpace,
                                MEMORY_AREA_NO_ACCESS,
                                &BaseAddress,
                                0x10000 - PAGE_SIZE,
                                PAGE_NOACCESS,
                                &MemoryArea,
                                FALSE,
                                0,
                                BoundaryAddressMultiple);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to protect the memory above the shared user page\n");
        goto exit;
     }

    /* Create the shared data page */
    BaseAddress = (PVOID)USER_SHARED_DATA;
    Status = MmCreateMemoryArea(ProcessAddressSpace,
                                MEMORY_AREA_SHARED_DATA,
                                &BaseAddress,
                                PAGE_SIZE,
                                PAGE_EXECUTE_READ,
                                &MemoryArea,
                                FALSE,
                                0,
                                BoundaryAddressMultiple);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create Shared User Data\n");
        goto exit;
     }

    /* The process now has an address space */
    Process->HasAddressSpace = TRUE;

    /* Check if there's a Section Object */
    if (SectionObject)
    {
        UNICODE_STRING FileName;
        PWCHAR szSrc;
        PCHAR szDest;
        USHORT lnFName = 0;

        /* Unlock the Address Space */
        DPRINT("Unlocking\n");
        MmUnlockAddressSpace(ProcessAddressSpace);

        DPRINT("Mapping process image. Section: %p, Process: %p, ImageBase: %p\n",
                 SectionObject, Process, &ImageBase);
        Status = MmMapViewOfSection(Section,
                                    (PEPROCESS)Process,
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
            return Status;
        }

        /* Save the pointer */
        Process->SectionBaseAddress = ImageBase;

        /* Determine the image file name and save it to EPROCESS */
        DPRINT("Getting Image name\n");
        FileName = SectionObject->FileObject->FileName;
        szSrc = (PWCHAR)(FileName.Buffer + FileName.Length);
        while (szSrc >= FileName.Buffer)
        {
            /* Make sure this isn't a backslash */
            if (*--szSrc == OBJ_NAME_PATH_SEPARATOR)
            {
                /* If so, stop it here */
                szSrc++;
                break;
            }
            else
            {
                /* Otherwise, keep going */
                lnFName++;
            }
        }

        /* Copy the to the process and truncate it to 15 characters if necessary */
        szDest = Process->ImageFileName;
        lnFName = min(lnFName, sizeof(Process->ImageFileName) - 1);
        while (lnFName--) *szDest++ = (UCHAR)*szSrc++;
        *szDest = UNICODE_NULL;

        /* Check if caller wants an audit name */
        if (AuditName)
        {
            /* Setup the audit name */
            SeInitializeProcessAuditName(SectionObject->FileObject,
                                         FALSE,
                                         AuditName);
        }

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

VOID
NTAPI
MmCleanProcessAddressSpace(IN PEPROCESS Process)
{
    /* FIXME: Add part of MmDeleteProcessAddressSpace here */
}

NTSTATUS
NTAPI
MmDeleteProcessAddressSpace(PEPROCESS Process)
{
   PVOID Address;
   PMEMORY_AREA MemoryArea;

   DPRINT("MmDeleteProcessAddressSpace(Process %x (%s))\n", Process,
          Process->ImageFileName);

   MmLockAddressSpace((PMADDRESS_SPACE)&Process->VadRoot);

   while ((MemoryArea = ((PMADDRESS_SPACE)&Process->VadRoot)->MemoryAreaRoot) != NULL)
   {
      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_SECTION_VIEW:
             Address = (PVOID)MemoryArea->StartingAddress;
             MmUnlockAddressSpace((PMADDRESS_SPACE)&Process->VadRoot);
             MmUnmapViewOfSection(Process, Address);
             MmLockAddressSpace((PMADDRESS_SPACE)&Process->VadRoot);
             break;

         case MEMORY_AREA_VIRTUAL_MEMORY:
         case MEMORY_AREA_PEB_OR_TEB:
             MmFreeVirtualMemory(Process, MemoryArea);
             break;

         case MEMORY_AREA_SHARED_DATA:
         case MEMORY_AREA_NO_ACCESS:
             MmFreeMemoryArea((PMADDRESS_SPACE)&Process->VadRoot,
                              MemoryArea,
                              NULL,
                              NULL);
             break;

         case MEMORY_AREA_MDL_MAPPING:
            KEBUGCHECK(PROCESS_HAS_LOCKED_PAGES);
            break;

         default:
            KEBUGCHECK(0);
      }
   }

   Mmi386ReleaseMmInfo(Process);

   MmUnlockAddressSpace((PMADDRESS_SPACE)&Process->VadRoot);
   MmDestroyAddressSpace((PMADDRESS_SPACE)&Process->VadRoot);

   DPRINT("Finished MmReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}
