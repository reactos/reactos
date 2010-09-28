/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/virtual.c
 * PURPOSE:         Implementing operations on virtual memory.
 *
 * PROGRAMMERS:     David Welch
 */

/* INCLUDE ********************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS FASTCALL
MiQueryVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID Address,
                     IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
                     OUT PVOID VirtualMemoryInformation,
                     IN SIZE_T Length,
                     OUT PSIZE_T ResultLength)
{
    NTSTATUS Status;
    PEPROCESS Process;
    MEMORY_AREA* MemoryArea;
    PMMSUPPORT AddressSpace;

    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       NULL,
                                       UserMode,
                                       (PVOID*)(&Process),
                                       NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryVirtualMemory() = %x\n",Status);
        return(Status);
    }

    AddressSpace = &Process->Vm;

    MmLockAddressSpace(AddressSpace);
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);
    switch(VirtualMemoryInformationClass)
    {
        case MemoryBasicInformation:
        {
            PMEMORY_BASIC_INFORMATION Info =
                (PMEMORY_BASIC_INFORMATION)VirtualMemoryInformation;
            if (Length != sizeof(MEMORY_BASIC_INFORMATION))
            {
                MmUnlockAddressSpace(AddressSpace);
                ObDereferenceObject(Process);
                return(STATUS_INFO_LENGTH_MISMATCH);
            }

            if (MemoryArea == NULL)
            {
                Info->Type = 0;
                Info->State = MEM_FREE;
                Info->Protect = PAGE_NOACCESS;
                Info->AllocationProtect = 0;
                Info->BaseAddress = (PVOID)PAGE_ROUND_DOWN(Address);
                Info->AllocationBase = NULL;
                Info->RegionSize = MmFindGapAtAddress(AddressSpace, Info->BaseAddress);
                Status = STATUS_SUCCESS;
                *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
            }
            else
            {
                switch(MemoryArea->Type)
                {
                    case MEMORY_AREA_VIRTUAL_MEMORY:
                        Status = MmQueryAnonMem(MemoryArea, Address, Info,
                                                ResultLength);
                        break;

                    case MEMORY_AREA_SECTION_VIEW:
                        Status = MmQuerySectionView(MemoryArea, Address, Info,
                                                    ResultLength);
                        break;

                    default:
                        DPRINT1("unhandled memory area type: 0x%x\n", MemoryArea->Type);
                        Status = STATUS_UNSUCCESSFUL;
                        *ResultLength = 0;
                }
            }
            break;
        }

        default:
        {
            DPRINT1("Unsupported or unimplemented class: %lx\n", VirtualMemoryInformationClass);
            Status = STATUS_INVALID_INFO_CLASS;
            *ResultLength = 0;
            break;
        }
    }

    MmUnlockAddressSpace(AddressSpace);
    ObDereferenceObject(Process);

    return Status;
}

NTSTATUS NTAPI
MiProtectVirtualMemory(IN PEPROCESS Process,
                       IN OUT PVOID *BaseAddress,
                       IN OUT PSIZE_T NumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG OldAccessProtection  OPTIONAL)
{
    PMEMORY_AREA MemoryArea;
    PMMSUPPORT AddressSpace;
    ULONG OldAccessProtection_;
    NTSTATUS Status;

    *NumberOfBytesToProtect =
    PAGE_ROUND_UP((ULONG_PTR)(*BaseAddress) + (*NumberOfBytesToProtect)) -
    PAGE_ROUND_DOWN(*BaseAddress);
    *BaseAddress = (PVOID)PAGE_ROUND_DOWN(*BaseAddress);

    AddressSpace = &Process->Vm;

    MmLockAddressSpace(AddressSpace);
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, *BaseAddress);
    if (MemoryArea == NULL)
    {
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_UNSUCCESSFUL;
    }

    if (OldAccessProtection == NULL)
        OldAccessProtection = &OldAccessProtection_;

    if (MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY)
    {
        Status = MmProtectAnonMem(AddressSpace, MemoryArea, *BaseAddress,
                                  *NumberOfBytesToProtect, NewAccessProtection,
                                  OldAccessProtection);
    }
    else if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
    {
        Status = MmProtectSectionView(AddressSpace, MemoryArea, *BaseAddress,
                                      *NumberOfBytesToProtect,
                                      NewAccessProtection,
                                      OldAccessProtection);
    }
    else
    {
        /* FIXME: Should we return failure or success in this case? */
        Status = STATUS_CONFLICTING_ADDRESSES;
    }

    MmUnlockAddressSpace(AddressSpace);

    return Status;
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS NTAPI
NtQueryVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID Address,
                     IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
                     OUT PVOID VirtualMemoryInformation,
                     IN SIZE_T Length,
                     OUT PSIZE_T UnsafeResultLength)
{
    NTSTATUS Status;
    SIZE_T ResultLength = 0;
    KPROCESSOR_MODE PreviousMode;
    WCHAR ModuleFileNameBuffer[MAX_PATH] = {0};
    UNICODE_STRING ModuleFileName;
    PMEMORY_SECTION_NAME SectionName = NULL;
    PEPROCESS Process;
    union
    {
        MEMORY_BASIC_INFORMATION BasicInfo;
    }
    VirtualMemoryInfo;

    DPRINT("NtQueryVirtualMemory(ProcessHandle %x, Address %x, "
           "VirtualMemoryInformationClass %d, VirtualMemoryInformation %x, "
           "Length %lu ResultLength %x)\n",ProcessHandle,Address,
           VirtualMemoryInformationClass,VirtualMemoryInformation,
           Length,ResultLength);

    PreviousMode =  ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWrite(VirtualMemoryInformation,
                          Length,
                          sizeof(ULONG_PTR));

            if (UnsafeResultLength) ProbeForWriteSize_t(UnsafeResultLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    if (Address >= MmSystemRangeStart)
    {
        DPRINT1("Invalid parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME: Move this inside MiQueryVirtualMemory */
    if (VirtualMemoryInformationClass == MemorySectionName)
    {
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_QUERY_INFORMATION,
                                           NULL,
                                           PreviousMode,
                                           (PVOID*)(&Process),
                                           NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtQueryVirtualMemory() = %x\n",Status);
            return(Status);
        }

        RtlInitEmptyUnicodeString(&ModuleFileName, ModuleFileNameBuffer, sizeof(ModuleFileNameBuffer));
        Status = MmGetFileNameForAddress(Address, &ModuleFileName);

        if (NT_SUCCESS(Status))
        {
            SectionName = VirtualMemoryInformation;
            if (PreviousMode != KernelMode)
            {
                _SEH2_TRY
                {
                    RtlInitUnicodeString(&SectionName->SectionFileName, SectionName->NameBuffer);
                    SectionName->SectionFileName.MaximumLength = Length;
                    RtlCopyUnicodeString(&SectionName->SectionFileName, &ModuleFileName);

                    if (UnsafeResultLength != NULL)
                    {
                        *UnsafeResultLength = ModuleFileName.Length;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            else
            {
                RtlInitUnicodeString(&SectionName->SectionFileName, SectionName->NameBuffer);
                SectionName->SectionFileName.MaximumLength = Length;
                RtlCopyUnicodeString(&SectionName->SectionFileName, &ModuleFileName);

                if (UnsafeResultLength != NULL)
                {
                    *UnsafeResultLength = ModuleFileName.Length;
                }
            }
        }
        ObDereferenceObject(Process);
        return Status;
    }
    else
    {
        Status = MiQueryVirtualMemory(ProcessHandle,
                                      Address,
                                      VirtualMemoryInformationClass,
                                      &VirtualMemoryInfo,
                                      Length,
                                      &ResultLength);
    }

    if (NT_SUCCESS(Status))
    {
        if (PreviousMode != KernelMode)
        {
            _SEH2_TRY
            {
                if (ResultLength > 0)
                {
                    ProbeForWrite(VirtualMemoryInformation,
                                  ResultLength,
                                  1);
                    RtlCopyMemory(VirtualMemoryInformation,
                                  &VirtualMemoryInfo,
                                  ResultLength);
                }
                if (UnsafeResultLength != NULL)
                {
                    *UnsafeResultLength = ResultLength;
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
        else
        {
            if (ResultLength > 0)
            {
                RtlCopyMemory(VirtualMemoryInformation,
                              &VirtualMemoryInfo,
                              ResultLength);
            }

            if (UnsafeResultLength != NULL)
            {
                *UnsafeResultLength = ResultLength;
            }
        }
    }

    return(Status);
}

/* EOF */
