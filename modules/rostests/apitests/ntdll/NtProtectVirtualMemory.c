/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the NtProtectVirtualMemory API
 * PROGRAMMERS:     Jérôme Gardou <jerome.gardou@reactos.org>
 *                  Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static
void
TestReadWrite(void)
{
    ULONG* allocationStart = NULL;
    NTSTATUS status;
    SIZE_T allocationSize;
    ULONG oldProtection;

    /* Reserve a page */
    allocationSize = PAGE_SIZE;
    status = NtAllocateVirtualMemory(NtCurrentProcess(),
        (void**)&allocationStart,
        0,
        &allocationSize,
        MEM_RESERVE,
        PAGE_NOACCESS);
    ok(NT_SUCCESS(status), "Reserving memory failed\n");

    /* Commit the page (RW) */
    status = NtAllocateVirtualMemory(NtCurrentProcess(),
        (void**)&allocationStart,
        0,
        &allocationSize,
        MEM_COMMIT,
        PAGE_READWRITE);
    ok(NT_SUCCESS(status), "Commiting memory failed\n");

    /* Try writing it */
    StartSeh()
    {
        *allocationStart = 0xFF;
    } EndSeh(STATUS_SUCCESS);

    /* Try reading it */
    StartSeh()
    {
        ok(*allocationStart == 0xFF, "Memory was not written\n");
    } EndSeh(STATUS_SUCCESS);

    /* Set it as read only */
    status = NtProtectVirtualMemory(NtCurrentProcess(),
        (void**)&allocationStart,
        &allocationSize,
        PAGE_READONLY,
        &oldProtection);
    ok(NT_SUCCESS(status), "NtProtectVirtualMemory failed.\n");
    ok(oldProtection == PAGE_READWRITE, "Expected PAGE_READWRITE, got %08lx.\n", oldProtection);

    /* Try writing it */
    StartSeh()
    {
        *allocationStart = 0xAA;
    } EndSeh(STATUS_ACCESS_VIOLATION);

    /* Try reading it */
    StartSeh()
    {
        ok(*allocationStart == 0xFF, "read-only memory were changed.\n");
    } EndSeh(STATUS_SUCCESS);

    /* Set it as no access */
    status = NtProtectVirtualMemory(NtCurrentProcess(),
        (void**)&allocationStart,
        &allocationSize,
        PAGE_NOACCESS,
        &oldProtection);
    ok(NT_SUCCESS(status), "NtProtectVirtualMemory failed.\n");
    ok(oldProtection == PAGE_READONLY, "Expected PAGE_READONLY, got %08lx.\n", oldProtection);

    /* Try writing it */
    StartSeh()
    {
        *allocationStart = 0xAA;
    } EndSeh(STATUS_ACCESS_VIOLATION);

    /* Try reading it */
    StartSeh()
    {
        ok(*allocationStart == 0, "Test should not go as far as this.\n");
    } EndSeh(STATUS_ACCESS_VIOLATION);

    /* Set it as readable again */
    status = NtProtectVirtualMemory(NtCurrentProcess(),
        (void**)&allocationStart,
        &allocationSize,
        PAGE_READONLY,
        &oldProtection);
    ok(NT_SUCCESS(status), "NtProtectVirtualMemory failed.\n");
    ok(oldProtection == PAGE_NOACCESS, "Expected PAGE_READONLY, got %08lx.\n", oldProtection);

    /* Try writing it */
    StartSeh()
    {
        *allocationStart = 0xAA;
    } EndSeh(STATUS_ACCESS_VIOLATION);

    /* Try reading it */
    StartSeh()
    {
        ok(*allocationStart == 0xFF, "Memory content was not preserved.\n");
    } EndSeh(STATUS_SUCCESS);

    /* Free memory */
    status = NtFreeVirtualMemory(NtCurrentProcess(),
        (void**)&allocationStart,
        &allocationSize,
        MEM_RELEASE);
    ok(NT_SUCCESS(status), "Failed freeing memory.\n");
}

/* Regression test for CORE-13311 */
static
void
TestFreeNoAccess(void)
{
    PVOID Mem;
    SIZE_T Size;
    NTSTATUS Status;
    ULONG Iteration, PageNumber;
    PUCHAR Page;
    ULONG OldProtection;

    for (Iteration = 0; Iteration < 50000; Iteration++)
    {
        Mem = NULL;
        Size = 16 * PAGE_SIZE;
        Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                         &Mem,
                                         0,
                                         &Size,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        ok_ntstatus(Status, STATUS_SUCCESS);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        for (PageNumber = 0; PageNumber < 16; PageNumber++)
        {
            Page = Mem;
            Page += PageNumber * PAGE_SIZE;
            ok(*Page == 0,
               "[%lu, %lu] Got non-zero memory. %x at %p\n",
               Iteration, PageNumber, *Page, Page);
            *Page = 123;
        }

        Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                        &Mem,
                                        &Size,
                                        PAGE_NOACCESS,
                                        &OldProtection);
        ok_ntstatus(Status, STATUS_SUCCESS);
        ok_hex(OldProtection, PAGE_READWRITE);

        Size = 0;
        Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                     &Mem,
                                     &Size,
                                     MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }
}

START_TEST(NtProtectVirtualMemory)
{
    TestReadWrite();
    TestFreeNoAccess();
}
