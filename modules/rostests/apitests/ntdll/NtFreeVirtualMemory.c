/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for NtFreeVirtualMemory
 * COPYRIGHT:   Copyright 2011 Jérôme Gardou <jerome.gardou@reactos.org>
 *              Copyright 2017 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include "precomp.h"

static void Test_NtFreeVirtualMemory(void)
{
    PVOID Buffer = NULL, Buffer2;
    SIZE_T Length = PAGE_SIZE;
    NTSTATUS Status;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Buffer,
                                     0,
                                     &Length,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(((ULONG_PTR)Buffer % PAGE_SIZE) == 0, "The buffer is not aligned to PAGE_SIZE.\n");

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer,
                                 &Length,
                                 MEM_DECOMMIT);
    ok(Status == STATUS_SUCCESS, "NtFreeVirtualMemory failed : 0x%08lx\n", Status);

    /* Now try to free more than we got */
    Length++;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer,
                                 &Length,
                                 MEM_DECOMMIT);
    ok(Status == STATUS_UNABLE_TO_FREE_VM, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer,
                                 &Length,
                                 MEM_RELEASE);
    ok(Status == STATUS_UNABLE_TO_FREE_VM, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);

    /* Free out of bounds from the wrong origin */
    Length = PAGE_SIZE;
    Buffer2 = (PVOID)((ULONG_PTR)Buffer+1);

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_DECOMMIT);
    ok(Status == STATUS_UNABLE_TO_FREE_VM, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);

    Buffer2 = (PVOID)((ULONG_PTR)Buffer+1);
    Length = PAGE_SIZE;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_RELEASE);
    ok(Status == STATUS_UNABLE_TO_FREE_VM, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);

    /* Same but in bounds */
    Length = PAGE_SIZE - 1;
    Buffer2 = (PVOID)((ULONG_PTR)Buffer+1);

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_DECOMMIT);
    ok(Status == STATUS_SUCCESS, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);
    ok(Buffer2 == Buffer, "NtFreeVirtualMemory set wrong buffer.\n");
    ok(Length == PAGE_SIZE, "NtFreeVirtualMemory did not round Length to PAGE_SIZE.\n");

    Buffer2 = (PVOID)((ULONG_PTR)Buffer+1);
    Length = PAGE_SIZE-1;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_RELEASE);
    ok(Status == STATUS_SUCCESS, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);
    ok(Buffer2 == Buffer, "NtFreeVirtualMemory set wrong buffer.\n");
    ok(Length == PAGE_SIZE, "NtFreeVirtualMemory did not round Length to PAGE_SIZE.\n");

    /* Now allocate two pages and try to free them one after the other */
    Length = 2*PAGE_SIZE;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Buffer,
                                     0,
                                     &Length,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == 2*PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(((ULONG_PTR)Buffer % PAGE_SIZE) == 0, "The buffer is not aligned to PAGE_SIZE.\n");

    Buffer2 = Buffer;
    Length = PAGE_SIZE;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_RELEASE);
    ok(NT_SUCCESS(Status), "NtFreeVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(Buffer2 == Buffer, "The buffer is not aligned to PAGE_SIZE.\n");

    Buffer2 = (PVOID)((ULONG_PTR)Buffer+PAGE_SIZE);
    Length = PAGE_SIZE;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_RELEASE);
    ok(NT_SUCCESS(Status), "NtFreeVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(Buffer2 == (PVOID)((ULONG_PTR)Buffer+PAGE_SIZE), "The buffer is not aligned to PAGE_SIZE.\n");

    /* Same, but try to free the second page before the first one */
    Length = 2*PAGE_SIZE;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Buffer,
                                     0,
                                     &Length,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == 2*PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(((ULONG_PTR)Buffer % PAGE_SIZE) == 0, "The buffer is not aligned to PAGE_SIZE.\n");

    Buffer2 = (PVOID)((ULONG_PTR)Buffer+PAGE_SIZE);
    Length = PAGE_SIZE;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_RELEASE);
    ok(NT_SUCCESS(Status), "NtFreeVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(Buffer2 == (PVOID)((ULONG_PTR)Buffer+PAGE_SIZE), "The buffer is not aligned to PAGE_SIZE.\n");

    Buffer2 = Buffer;
    Length = PAGE_SIZE;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_RELEASE);
    ok(NT_SUCCESS(Status), "NtFreeVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(Buffer2 == Buffer, "The buffer is not aligned to PAGE_SIZE.\n");

    /* Now allocate two pages and try to free them in the middle */
    Length = 2*PAGE_SIZE;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Buffer,
                                     0,
                                     &Length,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == 2*PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(((ULONG_PTR)Buffer % PAGE_SIZE) == 0, "The buffer is not aligned to PAGE_SIZE.\n");

    Buffer2 = (PVOID)((ULONG_PTR)Buffer+1);
    Length = PAGE_SIZE;
    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &Buffer2,
                                 &Length,
                                 MEM_RELEASE);
    ok(NT_SUCCESS(Status), "NtFreeVirtualMemory failed : 0x%08lx\n", Status);
    ok(Length == 2*PAGE_SIZE, "Length mismatch : 0x%08lx\n", (ULONG)Length);
    ok(Buffer2 == Buffer, "The buffer is not aligned to PAGE_SIZE.\n");
}

static void Test_NtFreeVirtualMemory_Parameters(void)
{
    NTSTATUS Status;
    ULONG FreeType;
    int i;

    // 4th parameter: "ULONG FreeType".

    // A type is mandatory.
    Status = NtFreeVirtualMemory(NULL, NULL, NULL, 0ul);
    ok(Status == STATUS_INVALID_PARAMETER_4, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);

    // All but MEM_DECOMMIT and MEM_RELEASE are unsupported.
    // Each bit one by one.
    for (i = 0; i < 32; ++i)
    {
        FreeType = 1 << i;
        if (FreeType == MEM_DECOMMIT || FreeType == MEM_RELEASE)
            continue;

        Status = NtFreeVirtualMemory(NULL, NULL, NULL, FreeType);
        ok(Status == STATUS_INVALID_PARAMETER_4, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);
    }
    // All bits at once.
    // Not testing all other values.
    Status = NtFreeVirtualMemory(NULL, NULL, NULL, ~(MEM_DECOMMIT | MEM_RELEASE));
    ok(Status == STATUS_INVALID_PARAMETER_4, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);
    Status = NtFreeVirtualMemory(NULL, NULL, NULL, ~MEM_DECOMMIT);
    ok(Status == STATUS_INVALID_PARAMETER_4, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);
    Status = NtFreeVirtualMemory(NULL, NULL, NULL, ~MEM_RELEASE);
    ok(Status == STATUS_INVALID_PARAMETER_4, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);
    Status = NtFreeVirtualMemory(NULL, NULL, NULL, ~0ul);
    ok(Status == STATUS_INVALID_PARAMETER_4, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);

    // MEM_DECOMMIT and MEM_RELEASE are exclusive.
    Status = NtFreeVirtualMemory(NULL, NULL, NULL, MEM_DECOMMIT | MEM_RELEASE);
    ok(Status == STATUS_INVALID_PARAMETER_4, "NtFreeVirtualMemory returned status : 0x%08lx\n", Status);
}

START_TEST(NtFreeVirtualMemory)
{
    Test_NtFreeVirtualMemory();
    Test_NtFreeVirtualMemory_Parameters();
}
