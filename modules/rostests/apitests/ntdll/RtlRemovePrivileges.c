/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         See COPYING in the top level directory
 * PURPOSE:         Test for RtlRemovePrivileges
 * PROGRAMMER:      Ratin Gao <ratin@knsoft.org>
 */

#include "precomp.h"

START_TEST(RtlRemovePrivileges)
{
#if (NTDDI_VERSION >= NTDDI_VISTA)
    NTSTATUS Status;
    HANDLE TokenHandle, TestTokenHandle;
    ULONG ReturnLength;
    UCHAR Buffer
        [sizeof(TOKEN_PRIVILEGES) +
         sizeof(LUID_AND_ATTRIBUTES) * (SE_MAX_WELL_KNOWN_PRIVILEGE - SE_MIN_WELL_KNOWN_PRIVILEGE)];
    PTOKEN_PRIVILEGES Privileges;
    ULONG PrivilegesToKeep[2];

    /* Duplicate current process token to run this test */
    Status = NtOpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        ok(0, "Failed to open current process token with TOKEN_DUPLICATE access (Status code %lx)!\n", Status);
        return;
    }

    Status = NtDuplicateToken(TokenHandle, TOKEN_ALL_ACCESS, NULL, FALSE, TokenPrimary, &TestTokenHandle);
    NtClose(TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        ok(0, "Failed to duplicate current process token (Status code %lx)!\n", Status);
        return;
    }

    /* Retrieve token privileges, we need at least 3 privileges to run following tests */
    Status = NtQueryInformationToken(TestTokenHandle, TokenPrivileges, Buffer, sizeof(Buffer), &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        NtClose(TestTokenHandle);
        ok(0, "Failed to retrieve token privileges (Status code %lx)!\n", Status);
        return;
    }
    Privileges = (PTOKEN_PRIVILEGES)Buffer;
    if (Privileges->PrivilegeCount < 3)
    {
        NtClose(TestTokenHandle);
        ok(0, "No enough privileges to run the test (Number of privilege: %lu)!\n", Privileges->PrivilegeCount);
        return;
    }

    /* Remove all privileges except 2nd and 3rd privileges, this should succeed */
    PrivilegesToKeep[0] = Privileges->Privileges[1].Luid.LowPart;
    PrivilegesToKeep[1] = Privileges->Privileges[2].Luid.LowPart;
    Status = RtlRemovePrivileges(TestTokenHandle, PrivilegesToKeep, ARRAYSIZE(PrivilegesToKeep));

    /* Do not use NT_SUCCESS, RtlRemovePrivileges may returns STATUS_NOT_ALL_ASSIGNED */
    if (Status != STATUS_SUCCESS)
    {
        NtClose(TestTokenHandle);
        ok_ntstatus(Status, STATUS_SUCCESS);
        return;
    }

    /* Now, only two privileges we kept should be present */
    Status = NtQueryInformationToken(TestTokenHandle, TokenPrivileges, Buffer, sizeof(Buffer), &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        NtClose(TestTokenHandle);
        ok(0, "Failed to retrieve token privileges (Status code %lx)!\n", Status);
        return;
    }
    ok(Privileges->PrivilegeCount == ARRAYSIZE(PrivilegesToKeep),
       "Number of privileges after RtlRemovePrivileges is %lu, expected %u\n", Privileges->PrivilegeCount,
       ARRAYSIZE(PrivilegesToKeep));
    ok(PrivilegesToKeep[0] + PrivilegesToKeep[1] ==
           Privileges->Privileges[0].Luid.LowPart + Privileges->Privileges[1].Luid.LowPart,
       "Incorrect privileges kept by RtlRemovePrivileges: %lu and %lu, expected %lu and %lu",
       Privileges->Privileges[0].Luid.LowPart, Privileges->Privileges[1].Luid.LowPart, PrivilegesToKeep[0],
       PrivilegesToKeep[1]);

    /* Remove all privileges, this should succeed */
    Status = RtlRemovePrivileges(TestTokenHandle, NULL, 0);

    /* Do not use NT_SUCCESS, RtlRemovePrivileges may returns STATUS_NOT_ALL_ASSIGNED */
    if (Status != STATUS_SUCCESS)
    {
        NtClose(TestTokenHandle);
        ok_ntstatus(Status, STATUS_SUCCESS);
        return;
    }

    /* Now, no privilege should be present */
    Status = NtQueryInformationToken(TestTokenHandle, TokenPrivileges, Buffer, sizeof(Buffer), &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        NtClose(TestTokenHandle);
        ok(0, "Failed to retrieve token privileges (Status code %lx)!\n", Status);
        return;
    }
    ok(Privileges->PrivilegeCount == 0, "There are %lu privileges still exist after RtlRemovePrivileges\n",
       Privileges->PrivilegeCount);

    NtClose(TestTokenHandle);
    return;
#else
    skip("RtlRemovePrivileges available on NT6.0+ (NTDDI_VERSION >= NTDDI_VISTA)");
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */
}
