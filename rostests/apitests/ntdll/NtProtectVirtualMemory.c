/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the NtProtectVirtualMemory API
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>
#include <ndk/mmfuncs.h>

START_TEST(NtProtectVirtualMemory)
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
