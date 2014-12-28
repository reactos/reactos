/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

HANDLE GlobalUserHeap = NULL;
PVOID GlobalUserHeapSection = NULL;


_Function_class_(RTL_HEAP_COMMIT_ROUTINE)
_IRQL_requires_same_
static
NTSTATUS
NTAPI
IntUserHeapCommitRoutine(
    _In_ PVOID Base,
    _Inout_ PVOID *CommitAddress,
    _Inout_ PSIZE_T CommitSize)
{
    PPROCESSINFO W32Process;
    PW32HEAP_USER_MAPPING Mapping;
    PVOID UserBase = NULL;
    NTSTATUS Status;
    SIZE_T Delta;
    PVOID UserCommitAddress;

    W32Process = PsGetCurrentProcessWin32Process();

    if (W32Process != NULL)
    {
        /* Search for the mapping */
        Mapping = &W32Process->HeapMappings;
        while (Mapping != NULL)
        {
            if (Mapping->KernelMapping == Base)
            {
                UserBase = Mapping->UserMapping;
                break;
            }

            Mapping = Mapping->Next;
        }

        ASSERT(UserBase != NULL);
    }
    else
    {
        SIZE_T ViewSize = 0;
        LARGE_INTEGER Offset;

        /* HACK: This needs to be handled during startup only... */
        ASSERT(Base == (PVOID)GlobalUserHeap);

        /* Temporarily map it into user space */
        Offset.QuadPart = 0;
        Status = MmMapViewOfSection(GlobalUserHeapSection,
                                    PsGetCurrentProcess(),
                                    &UserBase,
                                    0,
                                    0,
                                    &Offset,
                                    &ViewSize,
                                    ViewUnmap,
                                    SEC_NO_CHANGE,
                                    PAGE_EXECUTE_READ); /* Would prefer PAGE_READONLY, but thanks to RTL heaps... */

        if (!NT_SUCCESS(Status))
            return Status;
    }

    /* Apply the commit address offset to the user base address */
    Delta = (SIZE_T)((ULONG_PTR)(*CommitAddress) - (ULONG_PTR)Base);
    UserCommitAddress = (PVOID)((ULONG_PTR)UserBase + Delta);

    /* Perform the actual commit */
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &UserCommitAddress,
                                     0,
                                     CommitSize,
                                     MEM_COMMIT,
                                     PAGE_EXECUTE_READ);

    if (NT_SUCCESS(Status))
    {
        /* Determine the address to return */
        Delta = (SIZE_T)((ULONG_PTR)UserCommitAddress - (ULONG_PTR)UserBase);
        *CommitAddress = (PVOID)((ULONG_PTR)Base + Delta);
    }

    if (W32Process == NULL)
    {
        MmUnmapViewOfSection(PsGetCurrentProcess(),
                             UserBase);
    }

    return Status;
}

static PWIN32HEAP
IntUserHeapCreate(IN PVOID SectionObject,
                  IN PVOID *SystemMappedBase,
                  IN ULONG HeapSize)
{
    PVOID MappedView = NULL;
    LARGE_INTEGER Offset;
    SIZE_T ViewSize = PAGE_SIZE;
    RTL_HEAP_PARAMETERS Parameters = {0};
    PVOID pHeap;
    NTSTATUS Status;

    Offset.QuadPart = 0;

    /* Commit the first page before creating the heap! */
    Status = MmMapViewOfSection(SectionObject,
                                PsGetCurrentProcess(),
                                &MappedView,
                                0,
                                0,
                                &Offset,
                                &ViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_EXECUTE_READ); /* Would prefer PAGE_READONLY, but thanks to RTL heaps... */
    if (!NT_SUCCESS(Status))
        return NULL;

    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &MappedView,
                                     0,
                                     &ViewSize,
                                     MEM_COMMIT,
                                     PAGE_EXECUTE_READ); /* Would prefer PAGE_READONLY, but thanks to RTL heaps... */

    MmUnmapViewOfSection(PsGetCurrentProcess(),
                         MappedView);

    if (!NT_SUCCESS(Status))
        return NULL;

    /* Create the heap, don't serialize in kmode! The caller is responsible
       to synchronize the heap! */
    Parameters.Length = sizeof(Parameters);
    Parameters.InitialCommit = ViewSize;
    Parameters.InitialReserve = (SIZE_T)HeapSize;
    Parameters.CommitRoutine = IntUserHeapCommitRoutine;

    pHeap = RtlCreateHeap(HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE,
                          *SystemMappedBase,
                          (SIZE_T)HeapSize,
                          ViewSize,
                          NULL,
                          &Parameters);

    return pHeap;
}

PWIN32HEAP
UserCreateHeap(OUT PVOID *SectionObject,
               IN OUT PVOID *SystemBase,
               IN SIZE_T HeapSize)
{
    LARGE_INTEGER SizeHeap;
    PWIN32HEAP pHeap = NULL;
    NTSTATUS Status;

    SizeHeap.QuadPart = HeapSize;

    /* Create the section and map it into session space */
    Status = MmCreateSection((PVOID*)SectionObject,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &SizeHeap,
                             PAGE_EXECUTE_READWRITE, /* Would prefer PAGE_READWRITE, but thanks to RTL heaps... */
                             SEC_RESERVE | 1,
                             NULL,
                             NULL);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    Status = MmMapViewInSessionSpace(*SectionObject,
                                     SystemBase,
                                     &HeapSize);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(*SectionObject);
        *SectionObject = NULL;

        SetLastNtError(Status);
        return FALSE;
    }

    /* Create the heap */
    pHeap = IntUserHeapCreate(*SectionObject,
                              SystemBase,
                              HeapSize);

    if (pHeap == NULL)
    {
        ObDereferenceObject(*SectionObject);
        *SectionObject = NULL;

        SetLastNtError(STATUS_UNSUCCESSFUL);
    }

    return pHeap;
}
