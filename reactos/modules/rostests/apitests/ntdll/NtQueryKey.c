/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for the NtQueryKey API
 * PROGRAMMER:      Jérôme Gardou <jerome.gardou@reactos.org>
 *                  Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/cmtypes.h>
#include <ndk/obfuncs.h>

static
void
Test_KeyFullInformation(void)
{
    UNICODE_STRING HKLM_Name = RTL_CONSTANT_STRING(L"\\Registry\\Machine");
    UNICODE_STRING Software_Name = RTL_CONSTANT_STRING(L"Software");
    UNICODE_STRING Test_Name = RTL_CONSTANT_STRING(L"NtQueryKey_apitest");
    UNICODE_STRING MyClass = RTL_CONSTANT_STRING(L"MyClass");
    HANDLE HKLM_Key, HKLM_Software_Key, Test_Key;
    ULONG FullInformationLength;
    PKEY_FULL_INFORMATION FullInformation;
    ULONG InfoLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    FullInformationLength = FIELD_OFFSET(KEY_FULL_INFORMATION, Class[100]);
    FullInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, FullInformationLength);
    if (!FullInformation)
    {
        skip("Out of memory\n");
        return;
    }

    InitializeObjectAttributes(&ObjectAttributes,
        &HKLM_Name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    Status = NtOpenKey(&HKLM_Key, KEY_READ, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);

    InfoLength = 0x55555555;
    Status = NtQueryKey(HKLM_Key, KeyFullInformation, NULL, 0, &InfoLength);
    ok(Status == STATUS_BUFFER_TOO_SMALL, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "InfoLength = %lu\n", InfoLength);

    RtlFillMemory(FullInformation, FullInformationLength, 0x55);
    InfoLength = 0x55555555;
    Status = NtQueryKey(HKLM_Key, KeyFullInformation, FullInformation, FIELD_OFFSET(KEY_FULL_INFORMATION, Class), &InfoLength);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "InfoLength = %lu\n", InfoLength);
    ok(FullInformation->LastWriteTime.QuadPart != 0x5555555555555555, "LastWriteTime unchanged\n");
    ok(FullInformation->TitleIndex == 0, "TitleIndex = %lu\n", FullInformation->TitleIndex);
    ok(FullInformation->ClassOffset == 0xffffffff, "ClassOffset = %lu\n", FullInformation->ClassOffset);
    ok(FullInformation->ClassLength == 0, "ClassLength = %lu\n", FullInformation->ClassLength);
    ok(FullInformation->SubKeys >= 5 && FullInformation->SubKeys < 20, "SubKeys = %lu\n", FullInformation->SubKeys);
    ok(FullInformation->MaxNameLen >= 8 * sizeof(WCHAR) && FullInformation->MaxNameLen < 100 * sizeof(WCHAR), "MaxNameLen = %lu\n", FullInformation->MaxNameLen);
    ok(FullInformation->MaxClassLen != 0x55555555 && FullInformation->MaxClassLen % sizeof(WCHAR) == 0, "MaxClassLen = %lu\n", FullInformation->MaxClassLen);
    ok(FullInformation->Values != 0x55555555, "Values = %lu\n", FullInformation->Values);
    ok(FullInformation->MaxValueNameLen != 0x55555555 && FullInformation->MaxValueNameLen % sizeof(WCHAR) == 0, "MaxValueNameLen = %lu\n", FullInformation->MaxValueNameLen);
    ok(FullInformation->MaxValueDataLen != 0x55555555, "MaxValueDataLen = %lu\n", FullInformation->MaxValueDataLen);
    ok(FullInformation->Class[0] == 0x5555, "Class[0] = %u\n", FullInformation->Class[0]);

    RtlFillMemory(FullInformation, FullInformationLength, 0x55);
    InfoLength = 0x55555555;
    Status = NtQueryKey(HKLM_Key, KeyFullInformation, FullInformation, FIELD_OFFSET(KEY_FULL_INFORMATION, Class) - 1, &InfoLength);
    ok(Status == STATUS_BUFFER_TOO_SMALL, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "InfoLength = %lu\n", InfoLength);
    ok(FullInformation->LastWriteTime.QuadPart == 0x5555555555555555, "LastWriteTime changed: %I64d\n", FullInformation->LastWriteTime.QuadPart);

    InitializeObjectAttributes(&ObjectAttributes,
        &Software_Name,
        OBJ_CASE_INSENSITIVE,
        HKLM_Key,
        NULL);
    Status = NtOpenKey(&HKLM_Software_Key, KEY_READ, &ObjectAttributes);
    ok_ntstatus(Status, STATUS_SUCCESS);

    RtlFillMemory(FullInformation, FullInformationLength, 0x55);
    InfoLength = 0x55555555;
    Status = NtQueryKey(HKLM_Software_Key, KeyFullInformation, FullInformation, FIELD_OFFSET(KEY_FULL_INFORMATION, Class), &InfoLength);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "InfoLength = %lu\n", InfoLength);
    ok(FullInformation->LastWriteTime.QuadPart != 0x5555555555555555, "LastWriteTime unchanged\n");
    ok(FullInformation->TitleIndex == 0, "TitleIndex = %lu\n", FullInformation->TitleIndex);
    ok(FullInformation->ClassOffset == 0xffffffff, "ClassOffset = %lu\n", FullInformation->ClassOffset);
    ok(FullInformation->ClassLength == 0, "ClassLength = %lu\n", FullInformation->ClassLength);
    ok(FullInformation->SubKeys >= 5 && FullInformation->SubKeys < 1000, "SubKeys = %lu\n", FullInformation->SubKeys);
    ok(FullInformation->MaxNameLen >= 8 * sizeof(WCHAR), "MaxNameLen = %lu\n", FullInformation->MaxNameLen);
    ok(FullInformation->MaxClassLen != 0x55555555 && FullInformation->MaxClassLen % sizeof(WCHAR) == 0, "MaxClassLen = %lu\n", FullInformation->MaxClassLen);
    ok(FullInformation->Values != 0x55555555, "Values = %lu\n", FullInformation->Values);
    ok(FullInformation->MaxValueNameLen != 0x55555555 && FullInformation->MaxValueNameLen % sizeof(WCHAR) == 0, "MaxValueNameLen = %lu\n", FullInformation->MaxValueNameLen);
    ok(FullInformation->MaxValueDataLen != 0x55555555, "MaxValueDataLen = %lu\n", FullInformation->MaxValueDataLen);
    ok(FullInformation->Class[0] == 0x5555, "Class[0] = %u\n", FullInformation->Class[0]);

    InitializeObjectAttributes(&ObjectAttributes,
        &Test_Name,
        OBJ_CASE_INSENSITIVE,
        HKLM_Software_Key,
        NULL);
    Status = NtCreateKey(&Test_Key, KEY_ALL_ACCESS, &ObjectAttributes, 0, &MyClass, REG_OPTION_VOLATILE, NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    InfoLength = 0x55555555;
    Status = NtQueryKey(Test_Key, KeyFullInformation, NULL, 0, &InfoLength);
    ok(Status == STATUS_BUFFER_TOO_SMALL, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length, "InfoLength = %lu\n", InfoLength);

    RtlFillMemory(FullInformation, FullInformationLength, 0x55);
    InfoLength = 0x55555555;
    Status = NtQueryKey(Test_Key, KeyFullInformation, FullInformation, FIELD_OFFSET(KEY_FULL_INFORMATION, Class), &InfoLength);
    ok(Status == STATUS_BUFFER_OVERFLOW, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length, "InfoLength = %lu\n", InfoLength);
    ok(FullInformation->LastWriteTime.QuadPart != 0x5555555555555555, "LastWriteTime unchanged\n");
    ok(FullInformation->TitleIndex == 0, "TitleIndex = %lu\n", FullInformation->TitleIndex);
    ok(FullInformation->ClassOffset == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "ClassOffset = %lu\n", FullInformation->ClassOffset);
    ok(FullInformation->ClassLength == MyClass.Length, "ClassLength = %lu\n", FullInformation->ClassLength);
    ok(FullInformation->SubKeys == 0, "SubKeys = %lu\n", FullInformation->SubKeys);
    ok(FullInformation->MaxNameLen == 0, "MaxNameLen = %lu\n", FullInformation->MaxNameLen);
    ok(FullInformation->MaxClassLen == 0, "MaxClassLen = %lu\n", FullInformation->MaxClassLen);
    ok(FullInformation->Values == 0, "Values = %lu\n", FullInformation->Values);
    ok(FullInformation->MaxValueNameLen == 0, "MaxValueNameLen = %lu\n", FullInformation->MaxValueNameLen);
    ok(FullInformation->MaxValueDataLen == 0, "MaxValueDataLen = %lu\n", FullInformation->MaxValueDataLen);
    ok(FullInformation->Class[0] == 0x5555, "Class[0] = %u\n", FullInformation->Class[0]);

    RtlFillMemory(FullInformation, FullInformationLength, 0x55);
    InfoLength = 0x55555555;
    Status = NtQueryKey(Test_Key, KeyFullInformation, FullInformation, FIELD_OFFSET(KEY_FULL_INFORMATION, Class[1]), &InfoLength);
    ok(Status == STATUS_BUFFER_OVERFLOW, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length, "InfoLength = %lu\n", InfoLength);
    ok(FullInformation->LastWriteTime.QuadPart != 0x5555555555555555, "LastWriteTime unchanged\n");
    ok(FullInformation->ClassOffset == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "ClassOffset = %lu\n", FullInformation->ClassOffset);
    ok(FullInformation->ClassLength == MyClass.Length, "ClassLength = %lu\n", FullInformation->ClassLength);
    ok(FullInformation->Class[0] == L'M', "Class[0] = %u\n", FullInformation->Class[0]);
    ok(FullInformation->Class[1] == 0x5555, "Class[1] = %u\n", FullInformation->Class[1]);

    RtlFillMemory(FullInformation, FullInformationLength, 0x55);
    InfoLength = 0x55555555;
    Status = NtQueryKey(Test_Key, KeyFullInformation, FullInformation, FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length - 1, &InfoLength);
    ok(Status == STATUS_BUFFER_OVERFLOW, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length, "InfoLength = %lu\n", InfoLength);
    ok(FullInformation->LastWriteTime.QuadPart != 0x5555555555555555, "LastWriteTime unchanged\n");
    ok(FullInformation->ClassOffset == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "ClassOffset = %lu\n", FullInformation->ClassOffset);
    ok(FullInformation->ClassLength == MyClass.Length, "ClassLength = %lu\n", FullInformation->ClassLength);
    ok(FullInformation->Class[0] == L'M', "Class[0] = %u\n", FullInformation->Class[0]);
    ok(FullInformation->Class[1] == L'y', "Class[1] = %u\n", FullInformation->Class[1]);
    ok(FullInformation->Class[6] == (L's' | 0x5500), "Class[6] = %u\n", FullInformation->Class[6]);
    ok(FullInformation->Class[7] == 0x5555, "Class[7] = %u\n", FullInformation->Class[7]);

    RtlFillMemory(FullInformation, FullInformationLength, 0x55);
    InfoLength = 0x55555555;
    Status = NtQueryKey(Test_Key, KeyFullInformation, FullInformation, FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length, &InfoLength);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length, "InfoLength = %lu\n", InfoLength);
    ok(FullInformation->LastWriteTime.QuadPart != 0x5555555555555555, "LastWriteTime unchanged\n");
    ok(FullInformation->ClassOffset == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "ClassOffset = %lu\n", FullInformation->ClassOffset);
    ok(FullInformation->ClassLength == MyClass.Length, "ClassLength = %lu\n", FullInformation->ClassLength);
    ok(FullInformation->Class[0] == L'M', "Class[0] = %u\n", FullInformation->Class[0]);
    ok(FullInformation->Class[1] == L'y', "Class[1] = %u\n", FullInformation->Class[1]);
    ok(FullInformation->Class[6] == L's', "Class[6] = %u\n", FullInformation->Class[6]);
    ok(FullInformation->Class[7] == 0x5555, "Class[7] = %u\n", FullInformation->Class[7]);

    RtlFillMemory(FullInformation, FullInformationLength, 0x55);
    InfoLength = 0x55555555;
    Status = NtQueryKey(Test_Key, KeyFullInformation, FullInformation, FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length + sizeof(UNICODE_NULL), &InfoLength);
    ok(Status == STATUS_SUCCESS, "Status = 0x%lx\n", Status);
    ok(InfoLength == FIELD_OFFSET(KEY_FULL_INFORMATION, Class) + MyClass.Length, "InfoLength = %lu\n", InfoLength);
    ok(FullInformation->LastWriteTime.QuadPart != 0x5555555555555555, "LastWriteTime unchanged\n");
    ok(FullInformation->ClassOffset == FIELD_OFFSET(KEY_FULL_INFORMATION, Class), "ClassOffset = %lu\n", FullInformation->ClassOffset);
    ok(FullInformation->ClassLength == MyClass.Length, "ClassLength = %lu\n", FullInformation->ClassLength);
    ok(FullInformation->Class[0] == L'M', "Class[0] = %u\n", FullInformation->Class[0]);
    ok(FullInformation->Class[1] == L'y', "Class[1] = %u\n", FullInformation->Class[1]);
    ok(FullInformation->Class[6] == L's', "Class[6] = %u\n", FullInformation->Class[6]);
    ok(FullInformation->Class[7] == 0x5555, "Class[7] = %u\n", FullInformation->Class[7]);

    RtlFreeHeap(RtlGetProcessHeap(), 0, FullInformation);

    Status = NtDeleteKey(Test_Key);
    ok_ntstatus(Status, STATUS_SUCCESS);

    NtClose(Test_Key);
    NtClose(HKLM_Software_Key);
    NtClose(HKLM_Key);
}

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
    if (!NameInformation)
    {
        skip("Out of memory\n");
        return;
    }

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
    Test_KeyFullInformation();
    Test_KeyNameInformation();
}
