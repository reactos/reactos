/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for NtCreateKey
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static
VOID
VerifyAccess_(
    _In_ HANDLE Handle,
    _In_ ACCESS_MASK ExpectedAccess,
    _In_ PCSTR File,
    _In_ INT Line)
{
    NTSTATUS Status;
    OBJECT_BASIC_INFORMATION BasicInfo;
    ULONG Length;

    Status = NtQueryObject(Handle,
                           ObjectBasicInformation,
                           &BasicInfo,
                           sizeof(BasicInfo),
                           &Length);
    ok_(File, Line)(Status == STATUS_SUCCESS, "NtQueryObject returned 0x%lx\n", Status);
    ok_(File, Line)(BasicInfo.GrantedAccess == ExpectedAccess,
                    "GrantedAccess is 0x%lx, expected 0x%lx\n",
                    BasicInfo.GrantedAccess, ExpectedAccess);
}
#define VerifyAccess(h, e) VerifyAccess_(h, e, __FILE__, __LINE__)

static
VOID
TestCreateOpen_(
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ACCESS_MASK ExpectedAccess,
    _In_ NTSTATUS ExpectedStatus,
    _In_ PCSTR File,
    _In_ INT Line)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software");
    OBJECT_ATTRIBUTES ObjectAttributes;

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         DesiredAccess,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL);
    ok_(File, Line)(Status == ExpectedStatus,
                    "NtCreateKey returned 0x%lx, expected 0x%lx\n",
                    Status, ExpectedStatus);
    if (NT_SUCCESS(Status))
    {
        VerifyAccess_(KeyHandle, ExpectedAccess, File, Line);
        Status = NtClose(KeyHandle);
        ok_(File, Line)(Status == STATUS_SUCCESS,
                        "NtClose from NtCreateKey returned 0x%lx\n",
                        Status);
    }
    else if (NT_SUCCESS(ExpectedStatus))
    {
        skip_(File, Line)("NtCreateKey failed, skipping\n");
    }

    Status = NtOpenKey(&KeyHandle,
                       DesiredAccess,
                       &ObjectAttributes);
    ok_(File, Line)(Status == ExpectedStatus,
                    "NtOpenKey returned 0x%lx, expected 0x%lx\n",
                    Status, ExpectedStatus);
    if (NT_SUCCESS(Status))
    {
        VerifyAccess_(KeyHandle, ExpectedAccess, File, Line);
        Status = NtClose(KeyHandle);
        ok_(File, Line)(Status == STATUS_SUCCESS,
                        "NtClose from NtOpenKey returned 0x%lx\n",
                        Status);
    }
    else if (NT_SUCCESS(ExpectedStatus))
    {
        skip_(File, Line)("NtOpenKey failed, skipping\n");
    }
}
#define TestCreateOpen(d, ea, es) TestCreateOpen_(d, ea, es, __FILE__, __LINE__)

START_TEST(NtCreateKey)
{
    TestCreateOpen(0, 0, STATUS_ACCESS_DENIED);
    TestCreateOpen(KEY_WOW64_32KEY, 0, STATUS_ACCESS_DENIED);
    TestCreateOpen(KEY_WOW64_64KEY, 0, STATUS_ACCESS_DENIED);
    TestCreateOpen(KEY_WOW64_32KEY | KEY_WOW64_64KEY, 0, STATUS_ACCESS_DENIED); // STATUS_INVALID_PARAMETER on Win7
    TestCreateOpen(READ_CONTROL, READ_CONTROL, STATUS_SUCCESS);
    TestCreateOpen(READ_CONTROL | KEY_WOW64_32KEY, READ_CONTROL, STATUS_SUCCESS);
    TestCreateOpen(READ_CONTROL | KEY_WOW64_64KEY, READ_CONTROL, STATUS_SUCCESS);
    TestCreateOpen(READ_CONTROL | KEY_WOW64_32KEY | KEY_WOW64_64KEY, READ_CONTROL, STATUS_SUCCESS); // STATUS_INVALID_PARAMETER on Win7
}
