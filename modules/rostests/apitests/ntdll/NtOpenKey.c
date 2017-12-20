/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for NtOpenKey data alignment
 * PROGRAMMER:      Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

#define TEST_STR    L"\\Registry\\Machine\\SOFTWARE"

START_TEST(NtOpenKey)
{
    OBJECT_ATTRIBUTES Object;
    UNICODE_STRING String;
    char GccShouldNotAlignThis[40 * 2];
    char GccShouldNotAlignThis2[20];
    PVOID Alias = GccShouldNotAlignThis + 1;
    PVOID UnalignedKey = GccShouldNotAlignThis2 + 1;

    HANDLE KeyHandle;
    NTSTATUS Status;

    memcpy(Alias, TEST_STR, sizeof(TEST_STR));


    RtlInitUnicodeString(&String, TEST_STR);
    InitializeObjectAttributes(&Object, &String, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &Object);
    ok_ntstatus(Status, STATUS_SUCCESS);

    if (!NT_SUCCESS(Status))
        return;

    NtClose(KeyHandle);

    String.Buffer = Alias;
    ok_hex(((ULONG_PTR)String.Buffer) % 2, 1);
    Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &Object);
    ok_ntstatus(Status, STATUS_DATATYPE_MISALIGNMENT);              // FIXME: Later windows versions succeed here.
    if (NT_SUCCESS(Status))
        NtClose(KeyHandle);

    RtlInitUnicodeString(&String, TEST_STR);
    ok_hex(((ULONG_PTR)UnalignedKey) % 2, 1);
    Status = NtOpenKey(UnalignedKey, KEY_QUERY_VALUE, &Object);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        NtClose(*(HANDLE*)(UnalignedKey));
    }
}
