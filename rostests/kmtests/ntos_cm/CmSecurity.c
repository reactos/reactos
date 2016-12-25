/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite NPFS security test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "../ntos_se/se.h"

#define CheckKeySecurity(name, AceCount, ...) CheckKeySecurity_(name, AceCount, __FILE__, __LINE__, ##__VA_ARGS__)
#define CheckKeySecurity_(name, AceCount, file, line, ...) CheckKeySecurity__(name, AceCount, file ":" KMT_STRINGIZE(line), ##__VA_ARGS__)
static
VOID
CheckKeySecurity__(
    _In_ PCWSTR KeyName,
    _In_ ULONG AceCount,
    _In_ PCSTR FileAndLine,
    ...)
{
    NTSTATUS Status;
    UNICODE_STRING KeyNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG SecurityDescriptorSize;
    PSID Owner;
    PSID Group;
    PACL Dacl;
    PACL Sacl;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    va_list Arguments;

    RtlInitUnicodeString(&KeyNameString, KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyNameString,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle,
                       READ_CONTROL | ACCESS_SYSTEM_SECURITY,
                       &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No key (%ls)\n", KeyName))
    {
        return;
    }

    Status = ZwQuerySecurityObject(KeyHandle,
                                   OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
                                   NULL,
                                   0,
                                   &SecurityDescriptorSize);
    ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
    if (skip(Status == STATUS_BUFFER_TOO_SMALL, "No security size (%ls)\n", KeyName))
    {
        ObCloseHandle(KeyHandle, KernelMode);
        return;
    }

    SecurityDescriptor = ExAllocatePoolWithTag(PagedPool,
                                               SecurityDescriptorSize,
                                               'dSmK');
    ok(SecurityDescriptor != NULL, "Failed to allocate %lu bytes\n", SecurityDescriptorSize);
    if (skip(SecurityDescriptor != NULL, "No memory for descriptor (%ls)\n", KeyName))
    {
        ObCloseHandle(KeyHandle, KernelMode);
        return;
    }

    Status = ZwQuerySecurityObject(KeyHandle,
                                   OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
                                   SecurityDescriptor,
                                   SecurityDescriptorSize,
                                   &SecurityDescriptorSize);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        Owner = NULL;
        Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
                                               &Owner,
                                               &Defaulted);
        CheckSid(Owner, NO_SIZE, SeExports->SeAliasAdminsSid);
        ok(Defaulted == FALSE, "Owner defaulted for %ls\n", KeyName);

        Group = NULL;
        Status = RtlGetGroupSecurityDescriptor(SecurityDescriptor,
                                               &Group,
                                               &Defaulted);
        CheckSid(Group, NO_SIZE, SeExports->SeLocalSystemSid);
        ok(Defaulted == FALSE, "Group defaulted for %ls\n", KeyName);

        Dacl = NULL;
        Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                              &Present,
                                              &Dacl,
                                              &Defaulted);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(Present == TRUE, "DACL not present for %ls\n", KeyName);
        ok(Defaulted == FALSE, "DACL defaulted for %ls\n", KeyName);
        va_start(Arguments, FileAndLine);
        VCheckAcl__(Dacl, AceCount, FileAndLine, Arguments);
        va_end(Arguments);

        Sacl = NULL;
        Status = RtlGetSaclSecurityDescriptor(SecurityDescriptor,
                                              &Present,
                                              &Sacl,
                                              &Defaulted);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(Present == FALSE, "SACL present for %ls\n", KeyName);
        ok(Defaulted == FALSE, "SACL defaulted for %ls\n", KeyName);
        ok(Sacl == NULL, "Sacl is %p for %ls\n", Sacl, KeyName);
    }
    ExFreePoolWithTag(SecurityDescriptor, 'dSmK');
    ObCloseHandle(KeyHandle, KernelMode);
}

START_TEST(CmSecurity)
{
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID TerminalServerSid;

    TerminalServerSid = ExAllocatePoolWithTag(PagedPool,
                                              RtlLengthRequiredSid(1),
                                              'iSmK');
    if (TerminalServerSid != NULL)
    {
        RtlInitializeSid(TerminalServerSid, &NtSidAuthority, 1);
        *RtlSubAuthoritySid(TerminalServerSid, 0) = SECURITY_TERMINAL_SERVER_RID;
    }
    CheckKeySecurity(L"\\REGISTRY",
                     4, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeWorldSid,       KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeRestrictedSid,  KEY_READ);

    CheckKeySecurity(L"\\REGISTRY\\MACHINE",
                     4, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeWorldSid,       KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeRestrictedSid,  KEY_READ);

    CheckKeySecurity(L"\\REGISTRY\\MACHINE\\HARDWARE",
                     4, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeWorldSid,       KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeRestrictedSid,  KEY_READ);

    CheckKeySecurity(L"\\REGISTRY\\MACHINE\\SAM",
                     4, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeWorldSid,       KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeRestrictedSid,  KEY_READ);

    CheckKeySecurity(L"\\REGISTRY\\MACHINE\\SECURITY",
                     2, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid, WRITE_DAC | READ_CONTROL);

    CheckKeySecurity(L"\\REGISTRY\\MACHINE\\SOFTWARE",
                    12, ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasUsersSid,      KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasUsersSid,      GENERIC_READ,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasPowerUsersSid, KEY_READ | KEY_WRITE | DELETE,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasPowerUsersSid, GENERIC_READ | GENERIC_WRITE | DELETE,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeLocalSystemSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeCreatorOwnerSid,    GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, TerminalServerSid,               KEY_READ | KEY_WRITE | DELETE,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, TerminalServerSid,               GENERIC_READ | GENERIC_WRITE | DELETE);

    CheckKeySecurity(L"\\REGISTRY\\MACHINE\\SYSTEM",
                    10, ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasUsersSid,      KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasUsersSid,      GENERIC_READ,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasPowerUsersSid, KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasPowerUsersSid, GENERIC_READ,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeLocalSystemSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeCreatorOwnerSid,    GENERIC_ALL);

    CheckKeySecurity(L"\\REGISTRY\\USER",
                     4, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid, KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeWorldSid,       KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SeExports->SeRestrictedSid,  KEY_READ);

    CheckKeySecurity(L"\\REGISTRY\\USER\\.DEFAULT",
                    10, ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasUsersSid,      KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasUsersSid,      GENERIC_READ,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasPowerUsersSid, KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasPowerUsersSid, GENERIC_READ,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeLocalSystemSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeCreatorOwnerSid,    GENERIC_ALL);

    CheckKeySecurity(L"\\REGISTRY\\USER\\S-1-5-18",
                    10, ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasUsersSid,      KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasUsersSid,      GENERIC_READ,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasPowerUsersSid, KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasPowerUsersSid, GENERIC_READ,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeAliasAdminsSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeLocalSystemSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeLocalSystemSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE, SeExports->SeCreatorOwnerSid,    GENERIC_ALL);

    CheckKeySecurity(L"\\REGISTRY\\USER\\S-1-5-20",
                     8, ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeNetworkServiceSid,  KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeLocalSystemSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeAliasAdminsSid,     KEY_ALL_ACCESS,
                        ACCESS_ALLOWED_ACE_TYPE,                     0, SeExports->SeRestrictedSid,      KEY_READ,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE |
                                                 OBJECT_INHERIT_ACE,    SeExports->SeNetworkServiceSid,  GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE |
                                                 OBJECT_INHERIT_ACE,    SeExports->SeLocalSystemSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE |
                                                 OBJECT_INHERIT_ACE,    SeExports->SeAliasAdminsSid,     GENERIC_ALL,
                        ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                 CONTAINER_INHERIT_ACE |
                                                 OBJECT_INHERIT_ACE,    SeExports->SeRestrictedSid,      GENERIC_READ);

    if (TerminalServerSid != NULL)
    {
        ExFreePoolWithTag(TerminalServerSid, 'iSmK');
    }
}
