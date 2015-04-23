/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for the NtQueryKey API
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/cmtypes.h>
#include <ndk/obfuncs.h>

static
void
Test_KeyNameInformation(void)
{
    UNICODE_STRING HKLM_Name = RTL_CONSTANT_STRING(L"\\Registry\\Machine");
    UNICODE_STRING HKLM_Software_Name = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software");
    UNICODE_STRING Software_Name = RTL_CONSTANT_STRING(L"Software");
    UNICODE_STRING InfoName;
    HANDLE HKLM_Key, HKLM_Software_Key;
    PKEY_NAME_INFORMATION NameInformation;
    ULONG InfoLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    /* Open the HKCU key */
    InitializeObjectAttributes(&ObjectAttributes,
        &HKLM_Name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    Status = NtOpenKey(&HKLM_Key, KEY_READ, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Get the name info length */
    InfoLength = 0;
    Status = NtQueryKey(HKLM_Key, KeyNameInformation, NULL, 0, &InfoLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_size_t(InfoLength, FIELD_OFFSET(KEY_NAME_INFORMATION, Name[HKLM_Name.Length/sizeof(WCHAR)]));

    /* Get it for real */
    NameInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, InfoLength);
    ok(NameInformation != NULL, "\n");

    Status = NtQueryKey(HKLM_Key, KeyNameInformation, NameInformation, InfoLength, &InfoLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_size_t(InfoLength, FIELD_OFFSET(KEY_NAME_INFORMATION, Name[HKLM_Name.Length/sizeof(WCHAR)]));
    ok_size_t(NameInformation->NameLength, HKLM_Name.Length);

    InfoName.Buffer = NameInformation->Name;
    InfoName.Length = NameInformation->NameLength;
    InfoName.MaximumLength = NameInformation->NameLength;
    ok(RtlCompareUnicodeString(&InfoName, &HKLM_Name, TRUE) == 0, "%.*S\n",
        InfoName.Length, InfoName.Buffer);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NameInformation);

    /* Open one subkey */
    InitializeObjectAttributes(&ObjectAttributes,
        &Software_Name,
        OBJ_CASE_INSENSITIVE,
        HKLM_Key,
        NULL);
    Status = NtOpenKey(&HKLM_Software_Key, KEY_READ, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Get the name info length */
    InfoLength = 0;
    Status = NtQueryKey(HKLM_Software_Key, KeyNameInformation, NULL, 0, &InfoLength);
    ok_ntstatus(Status, STATUS_BUFFER_TOO_SMALL);
    ok_size_t(InfoLength, FIELD_OFFSET(KEY_NAME_INFORMATION, Name[HKLM_Software_Name.Length/sizeof(WCHAR)]));

    /* Get it for real */
    NameInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, InfoLength);
    ok(NameInformation != NULL, "\n");

    Status = NtQueryKey(HKLM_Software_Key, KeyNameInformation, NameInformation, InfoLength, &InfoLength);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_size_t(InfoLength, FIELD_OFFSET(KEY_NAME_INFORMATION, Name[HKLM_Software_Name.Length/sizeof(WCHAR)]));
    ok_size_t(NameInformation->NameLength, HKLM_Software_Name.Length);

    InfoName.Buffer = NameInformation->Name;
    InfoName.Length = NameInformation->NameLength;
    InfoName.MaximumLength = NameInformation->NameLength;
    ok(RtlCompareUnicodeString(&InfoName, &HKLM_Software_Name, TRUE) == 0, "%.*S\n",
        InfoName.Length, InfoName.Buffer);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NameInformation);

    NtClose(HKLM_Software_Key);
    NtClose(HKLM_Key);
}

START_TEST(NtQueryKey)
{
    Test_KeyNameInformation();
}
