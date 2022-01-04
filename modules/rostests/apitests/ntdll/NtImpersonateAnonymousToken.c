/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtImpersonateAnonymousToken API
 * COPYRIGHT:       Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"
#include <winreg.h>

#define TOKEN_WITH_EVERYONE_GROUP    1
#define TOKEN_WITHOUT_EVERYONE_GROUP 0

static
HANDLE
GetThreadFromCurrentProcess(_In_ DWORD DesiredAccess)
{
    HANDLE Thread;

    Thread = OpenThread(DesiredAccess, FALSE, GetCurrentThreadId());
    if (!Thread)
    {
        skip("OpenThread() has failed to open the current process' thread (error code: %lu)\n", GetLastError());
        return NULL;
    }

    return Thread;
}

static
VOID
ImpersonateTokenWithEveryoneOrWithout(_In_ DWORD Value)
{
    LONG Result;
    HKEY Key;

    Result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\Lsa",
                           0,
                           KEY_SET_VALUE,
                           &Key);
    if (Result != ERROR_SUCCESS)
    {
        skip("RegOpenKeyExW() has failed to open the key (error code: %li)\n", Result);
        return;
    }

    Result = RegSetValueExW(Key,
                            L"EveryoneIncludesAnonymous",
                            0,
                            REG_DWORD,
                            (PBYTE)&Value,
                            sizeof(Value));
    if (Result != ERROR_SUCCESS)
    {
        skip("RegSetValueExW() has failed to set the value (error code: %li)\n", Result);
        RegCloseKey(Key);
        return;
    }

    RegCloseKey(Key);
}

START_TEST(NtImpersonateAnonymousToken)
{
    NTSTATUS Status;
    BOOL Success;
    HANDLE ThreadHandle;

    ThreadHandle = GetThreadFromCurrentProcess(THREAD_IMPERSONATE);

    /* We give an invalid thread handle */
    Status = NtImpersonateAnonymousToken(NULL);
    ok_hex(Status, STATUS_INVALID_HANDLE);

    /* We want to impersonate the token including Everyone Group SID */
    ImpersonateTokenWithEveryoneOrWithout(TOKEN_WITH_EVERYONE_GROUP);

    /* Impersonate the anonymous logon token */
    Status = NtImpersonateAnonymousToken(ThreadHandle);
    ok_hex(Status, STATUS_SUCCESS);

    /* Now revert to the previous security properties */
    Success = RevertToSelf();
    ok(Success == TRUE, "We should have terminated the impersonation but we couldn't (error code: %lu)\n", GetLastError());

    /* Return to default setting -- token without Everyone Group SID */
    ImpersonateTokenWithEveryoneOrWithout(TOKEN_WITHOUT_EVERYONE_GROUP);

    /* Impersonate the anonymous logon token again */
    Status = NtImpersonateAnonymousToken(ThreadHandle);
    ok_hex(Status, STATUS_SUCCESS);

    /* Now revert to the previous security properties */
    Success = RevertToSelf();
    ok(Success == TRUE, "We should have terminated the impersonation but we couldn't (error code: %lu)\n", GetLastError());

    /*
     * Invalidate the handle and open a new one. This time
     * with the wrong access right mask, the function will
     * outright fail on impersonating the token.
     */
    CloseHandle(ThreadHandle);
    ThreadHandle = GetThreadFromCurrentProcess(SYNCHRONIZE);

    /* The thread handle has incorrect right access */
    Status = NtImpersonateAnonymousToken(ThreadHandle);
    ok_hex(Status, STATUS_ACCESS_DENIED);

    /* We're done with the tests */
    CloseHandle(ThreadHandle);
}
