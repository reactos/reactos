#ifndef _APITEST_GUARD_H
#define _APITEST_GUARD_H

#include <ndk/mmfuncs.h>
#include <ndk/psfuncs.h>

static
inline
PVOID
AllocateGuarded(
    _In_ SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested + PAGE_SIZE);
    PVOID VirtualMemory = NULL;
    PCHAR StartOfBuffer;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_RESERVE, PAGE_NOACCESS);

    if (!NT_SUCCESS(Status))
        return NULL;

    Size -= PAGE_SIZE;
    if (Size)
    {
        Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            Size = 0;
            Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
            return NULL;
        }
    }

    StartOfBuffer = VirtualMemory;
    StartOfBuffer += Size - SizeRequested;

    return StartOfBuffer;
}

static
inline
VOID
FreeGuarded(
    _In_ PVOID Pointer)
{
    NTSTATUS Status;
    PVOID VirtualMemory = (PVOID)PAGE_ROUND_DOWN((SIZE_T)Pointer);
    SIZE_T Size = 0;

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
}

static
inline
VOID
MakeReadOnly(
    PVOID Pointer,
    SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested);
    PVOID VirtualMemory = (PVOID)PAGE_ROUND_DOWN((SIZE_T)Pointer);

    if (Size)
    {
        Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            Size = 0;
            Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
        }
    }
}

#endif /* _APITEST_GUARD_H */
