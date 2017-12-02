/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for NtDeleteKey
 */

#include "precomp.h"

static
NTSTATUS
CreateRegistryKeyHandle(PHANDLE KeyHandle,
                      ACCESS_MASK AccessMask,
                      PWCHAR RegistryPath)
{
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES Attributes;

    RtlInitUnicodeString(&KeyName, RegistryPath);
    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    return NtCreateKey(KeyHandle, AccessMask, &Attributes, 0, NULL, REG_OPTION_VOLATILE, 0);
}

START_TEST(NtDeleteKey)
{
    NTSTATUS Status;
    HANDLE ParentKey, ChildKey, PetKey;
    WCHAR Buffer[sizeof("\\Registry\\Machine\\Software\\RosTests\\Child\\Pet") + 5];
    ULONG i;

    /* Create a registry key */
    Status = CreateRegistryKeyHandle(&ParentKey, KEY_READ | DELETE, L"\\Registry\\Machine\\Software\\RosTests");
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Create a children registry key */
    Status = CreateRegistryKeyHandle(&ChildKey, KEY_READ | DELETE, L"\\Registry\\Machine\\Software\\RosTests\\Child");
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try deleting the parent one */
    Status = NtDeleteKey(ParentKey);
    ok_ntstatus(Status, STATUS_CANNOT_DELETE);

    /* See what happens with Child one */
    Status = NtDeleteKey(ChildKey);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Retry with parent one */
    Status = NtDeleteKey(ParentKey);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* re-retry */
    Status = NtDeleteKey(ParentKey);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Close everything */
    NtClose(ChildKey);
    NtClose(ParentKey);

    /* Same, but this time close the child handle before trying to delete the parent */
    /* Create a registry key */
    Status = CreateRegistryKeyHandle(&ParentKey, KEY_READ | DELETE, L"\\Registry\\Machine\\Software\\RosTests");
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Create a children registry key */
    Status = CreateRegistryKeyHandle(&ChildKey, KEY_READ, L"\\Registry\\Machine\\Software\\RosTests\\Child");
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Close the Child */
    NtClose(ChildKey);

    /* Try deleting the parent one */
    Status = NtDeleteKey(ParentKey);
    ok_ntstatus(Status, STATUS_CANNOT_DELETE);

    /* See what happens with Child one */
    Status = CreateRegistryKeyHandle(&ChildKey, DELETE, L"\\Registry\\Machine\\Software\\RosTests\\Child");
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtDeleteKey(ChildKey);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Retry with parent one */
    Status = NtDeleteKey(ParentKey);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Close everything */
    NtClose(ChildKey);
    NtClose(ParentKey);

    /* Stress test key creation */
    Status = CreateRegistryKeyHandle(&ParentKey, KEY_READ | DELETE, L"\\Registry\\Machine\\Software\\RosTests");

    for (i = 0; i <= 9999; i++) {
        swprintf(Buffer, L"\\Registry\\Machine\\Software\\RosTests\\Child%04d", i);
        Status = CreateRegistryKeyHandle(&ChildKey, KEY_READ, Buffer);
        NtClose(ChildKey);

        swprintf(Buffer, L"\\Registry\\Machine\\Software\\RosTests\\Child%04d\\Pet", i);
        Status = CreateRegistryKeyHandle(&PetKey, KEY_READ, Buffer);
        NtClose(PetKey);
    }
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Test hive rerooting */
    Status = CreateRegistryKeyHandle(&PetKey, DELETE, L"\\Registry\\Machine\\Software\\RosTests\\Child5000\\Pet");
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtDeleteKey(PetKey);
    ok_ntstatus(Status, STATUS_SUCCESS);

    NtClose(PetKey);

    Status = CreateRegistryKeyHandle(&ChildKey, DELETE, L"\\Registry\\Machine\\Software\\RosTests\\Child5000");
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtDeleteKey(ChildKey);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Test mass key deletion */
    for (i = 0; i <= 9999; i++) {
        if (i != 5000) {
            swprintf(Buffer, L"\\Registry\\Machine\\Software\\RosTests\\Child%04d\\Pet", i);
            CreateRegistryKeyHandle(&PetKey, DELETE, Buffer);
            Status = NtDeleteKey(PetKey);
            NtClose(PetKey);

            swprintf(Buffer, L"\\Registry\\Machine\\Software\\RosTests\\Child%04d", i);
            CreateRegistryKeyHandle(&ChildKey, DELETE, Buffer);
            Status = NtDeleteKey(ChildKey);
            NtClose(ChildKey);
        }
    }
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = NtDeleteKey(ParentKey);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Close ParentKey */
    NtClose(ParentKey);
}
