/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for NtOpenProcessToken[Ex]
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/sefuncs.h>

START_TEST(NtOpenProcessToken)
{
    NTSTATUS Status;
    HANDLE TokenHandle;

    Status = NtOpenProcessToken(NtCurrentProcess(),
                                TOKEN_READ,
                                &TokenHandle);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
    if (NT_SUCCESS(Status))
    {
        ok((LONG_PTR)TokenHandle > 0, "TokenHandle = %p\n", TokenHandle);
        Status = NtClose(TokenHandle);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
    }

    Status = NtOpenProcessTokenEx(NtCurrentProcess(),
                                  TOKEN_READ,
                                  0,
                                  &TokenHandle);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
    if (NT_SUCCESS(Status))
    {
        ok((LONG_PTR)TokenHandle > 0, "TokenHandle = %p\n", TokenHandle);
        Status = NtClose(TokenHandle);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
    }

    Status = NtOpenProcessTokenEx(NtCurrentProcess(),
                                  TOKEN_READ,
                                  OBJ_KERNEL_HANDLE,
                                  &TokenHandle);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
    if (NT_SUCCESS(Status))
    {
        ok((LONG_PTR)TokenHandle > 0, "TokenHandle = %p\n", TokenHandle);
        Status = NtClose(TokenHandle);
        ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
    }
}
