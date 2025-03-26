/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for the NtSetInformationToken API
 * COPYRIGHT:       Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"

static
HANDLE
OpenCurrentToken(VOID)
{
    BOOL Success;
    HANDLE Token;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_READ | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID,
                               &Token);
    if (!Success)
    {
        ok(0, "OpenProcessToken() has failed to get the process' token (error code: %lu)!\n", GetLastError());
        return NULL;
    }

    return Token;
}

static
PTOKEN_DEFAULT_DACL
QueryOriginalDefaultDacl(
    _In_ HANDLE Token,
    _Out_ PULONG DaclLength)
{
    NTSTATUS Status;
    PTOKEN_DEFAULT_DACL Dacl;
    ULONG BufferLength;

    *DaclLength = 0;

    Status = NtQueryInformationToken(Token,
                                     TokenDefaultDacl,
                                     NULL,
                                     0,
                                     &BufferLength);
    if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_TOO_SMALL))
    {
        ok(0, "Failed to query buffer length, STATUS_BUFFER_TOO_SMALL has to be expected (Status code %lx)!\n", Status);
        return NULL;
    }

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!Dacl)
    {
        ok(0, "Failed to allocate from heap for token default DACL (required buffer length %lu)!\n", BufferLength);
        return NULL;
    }

    Status = NtQueryInformationToken(Token,
                                     TokenDefaultDacl,
                                     Dacl,
                                     BufferLength,
                                     &BufferLength);
    if (!NT_SUCCESS(Status))
    {
        ok(0, "Failed to query default DACL (Status code %lx)!\n", Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
        return NULL;
    }

    *DaclLength = BufferLength;
    return Dacl;
}

static
PACL
CreateNewDefaultDacl(
    _Out_ PULONG DaclLength)
{
    NTSTATUS Status;
    PACL Dacl;
    ULONG Length;
    PSID LocalSystemSid;
    static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};

    *DaclLength = 0;

    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &LocalSystemSid);
    if (!NT_SUCCESS(Status))
    {
        ok(0, "Failed to allocate Local System SID (Status code %lx)!\n", Status);
        return NULL;
    }

    Length = sizeof(ACL) +
                 sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(LocalSystemSid);

    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           Length);
    if (!Dacl)
    {
        ok(0, "Failed to allocate from heap for DACL!\n");
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalSystemSid);
        return NULL;
    }

    Status = RtlCreateAcl(Dacl,
                          Length,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        ok(0, "Failed to create ACL (Status code %lx)!\n", Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalSystemSid);
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
        return NULL;
    }

    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    GENERIC_ALL,
                                    LocalSystemSid);
    if (!NT_SUCCESS(Status))
    {
        ok(0, "Failed to add access allowed ACE (Status code %lx)!\n", Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalSystemSid);
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
        return NULL;
    }

    *DaclLength = Length;
    RtlFreeHeap(RtlGetProcessHeap(), 0, LocalSystemSid);
    return Dacl;
}

static
VOID
SetTokenDefaultDaclTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    PACL NewDacl;
    TOKEN_DEFAULT_DACL NewDefaultDacl;
    PTOKEN_DEFAULT_DACL DefaultDacl;
    ULONG OriginalDaclLength, NewDaclLength;

    /*
     * Query the original DACL of the token first,
     * we don't want to leave the token tampered
     * later on.
     */
    DefaultDacl = QueryOriginalDefaultDacl(Token, &OriginalDaclLength);
    if (!DefaultDacl)
    {
        ok(0, "Failed to query token's default DACL!\n");
        return;
    }

    /* Allocate new DACL */
    NewDacl = CreateNewDefaultDacl(&NewDaclLength);
    if (!DefaultDacl)
    {
        ok(0, "Failed to allocate buffer for new DACL!\n");
        RtlFreeHeap(RtlGetProcessHeap(), 0, DefaultDacl);
        return;
    }

    NewDefaultDacl.DefaultDacl = NewDacl;

    /*
     * Set a new DACL for the token.
     */
    Status = NtSetInformationToken(Token,
                                   TokenDefaultDacl,
                                   &NewDefaultDacl,
                                   NewDaclLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Now set the original DACL */
    Status = NtSetInformationToken(Token,
                                   TokenDefaultDacl,
                                   DefaultDacl,
                                   OriginalDaclLength);
    ok_ntstatus(Status, STATUS_SUCCESS);

    RtlFreeHeap(RtlGetProcessHeap(), 0, DefaultDacl);
    RtlFreeHeap(RtlGetProcessHeap(), 0, NewDacl);
}

static
VOID
SetTokenSessionIdTests(
    _In_ HANDLE Token)
{
    NTSTATUS Status;
    ULONG SessionId = 1;

    /*
     * We're not allowed to set a session ID
     * because we don't have the TCB privilege.
     */
    Status = NtSetInformationToken(Token,
                                   TokenSessionId,
                                   &SessionId,
                                   sizeof(ULONG));
    ok_ntstatus(Status, STATUS_PRIVILEGE_NOT_HELD);
}

START_TEST(NtSetInformationToken)
{
    NTSTATUS Status;
    ULONG DummyReturnLength = 0;
    HANDLE Token;

    /* Everything else is NULL */
    Status = NtSetInformationToken(NULL,
                                   TokenOwner,
                                   NULL,
                                   0);
    ok_ntstatus(Status, STATUS_INVALID_HANDLE);

    /* We don't give a token */
    Status = NtSetInformationToken(NULL,
                                   TokenOwner,
                                   NULL,
                                   DummyReturnLength);
    ok_ntstatus(Status, STATUS_INVALID_HANDLE);

    Token = OpenCurrentToken();

    /* We give a bogus token class */
    Status = NtSetInformationToken(Token,
                                   0xa0a,
                                   NULL,
                                   DummyReturnLength);
    ok_ntstatus(Status, STATUS_INVALID_INFO_CLASS);

    /* Now perform tests for each class */
    SetTokenDefaultDaclTests(Token);
    SetTokenSessionIdTests(Token);

    CloseHandle(Token);
}
