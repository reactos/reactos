/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite object security test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "../ntos_se/se.h"

#define CheckDirectorySecurityWithOwnerAndGroup(name, Owner, Group, AceCount, ...) CheckDirectorySecurity_(name, Owner, Group, AceCount, __FILE__, __LINE__, ##__VA_ARGS__)
#define CheckDirectorySecurity(name, AceCount, ...) CheckDirectorySecurity_(name, SeExports->SeAliasAdminsSid, SeExports->SeLocalSystemSid, AceCount, __FILE__, __LINE__, ##__VA_ARGS__)
#define CheckDirectorySecurity_(name, Owner, Group, AceCount, file, line, ...) CheckDirectorySecurity__(name, Owner, Group, AceCount, file ":" KMT_STRINGIZE(line), ##__VA_ARGS__)
static
VOID
CheckDirectorySecurity__(
    _In_ PCWSTR DirectoryName,
    _In_ PSID ExpectedOwner,
    _In_ PSID ExpectedGroup,
    _In_ ULONG AceCount,
    _In_ PCSTR FileAndLine,
    ...)
{
    NTSTATUS Status;
    UNICODE_STRING DirectoryNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG SecurityDescriptorSize;
    PSID Owner;
    PSID Group;
    PACL Dacl;
    PACL Sacl;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    va_list Arguments;

    RtlInitUnicodeString(&DirectoryNameString, DirectoryName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirectoryNameString,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenDirectoryObject(&DirectoryHandle,
                                   READ_CONTROL | ACCESS_SYSTEM_SECURITY,
                                   &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No directory (%ls)\n", DirectoryName))
    {
        return;
    }

    Status = ZwQuerySecurityObject(DirectoryHandle,
                                   OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
                                   NULL,
                                   0,
                                   &SecurityDescriptorSize);
    ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
    if (skip(Status == STATUS_BUFFER_TOO_SMALL, "No security size (%ls)\n", DirectoryName))
    {
        ObCloseHandle(DirectoryHandle, KernelMode);
        return;
    }

    SecurityDescriptor = ExAllocatePoolWithTag(PagedPool,
                                               SecurityDescriptorSize,
                                               'dSmK');
    ok(SecurityDescriptor != NULL, "Failed to allocate %lu bytes\n", SecurityDescriptorSize);
    if (skip(SecurityDescriptor != NULL, "No memory for descriptor (%ls)\n", DirectoryName))
    {
        ObCloseHandle(DirectoryHandle, KernelMode);
        return;
    }

    Status = ZwQuerySecurityObject(DirectoryHandle,
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
        if (ExpectedOwner)
            CheckSid__(Owner, NO_SIZE, ExpectedOwner, FileAndLine);
        ok(Defaulted == FALSE, "Owner defaulted for %ls\n", DirectoryName);

        Group = NULL;
        Status = RtlGetGroupSecurityDescriptor(SecurityDescriptor,
                                               &Group,
                                               &Defaulted);
        if (ExpectedGroup)
            CheckSid__(Group, NO_SIZE, ExpectedGroup, FileAndLine);
        ok(Defaulted == FALSE, "Group defaulted for %ls\n", DirectoryName);

        Dacl = NULL;
        Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                              &Present,
                                              &Dacl,
                                              &Defaulted);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(Present == TRUE, "DACL not present for %ls\n", DirectoryName);
        ok(Defaulted == FALSE, "DACL defaulted for %ls\n", DirectoryName);
        va_start(Arguments, FileAndLine);
        VCheckAcl__(Dacl, AceCount, FileAndLine, Arguments);
        va_end(Arguments);

        Sacl = NULL;
        Status = RtlGetSaclSecurityDescriptor(SecurityDescriptor,
                                              &Present,
                                              &Sacl,
                                              &Defaulted);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok(Present == FALSE, "SACL present for %ls\n", DirectoryName);
        ok(Defaulted == FALSE, "SACL defaulted for %ls\n", DirectoryName);
        ok(Sacl == NULL, "Sacl is %p for %ls\n", Sacl, DirectoryName);
    }
    ExFreePoolWithTag(SecurityDescriptor, 'dSmK');
    ObCloseHandle(DirectoryHandle, KernelMode);
}

START_TEST(ObSecurity)
{
    NTSTATUS Status;
    /* Assume yes, that's the default on W2K3 */
    ULONG LUIDMappingsEnabled = 1, ReturnLength;

#define DIRECTORY_GENERIC_READ      STANDARD_RIGHTS_READ | DIRECTORY_TRAVERSE | DIRECTORY_QUERY
#define DIRECTORY_GENERIC_WRITE     STANDARD_RIGHTS_WRITE | DIRECTORY_CREATE_SUBDIRECTORY | DIRECTORY_CREATE_OBJECT

    /* Check if LUID device maps are enabled */
    Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessLUIDDeviceMapsEnabled,
                                       &LUIDMappingsEnabled,
                                       sizeof(LUIDMappingsEnabled),
                                       &ReturnLength);
    ok(NT_SUCCESS(Status), "NtQueryInformationProcess failed: 0x%x\n", Status);

    trace("LUID mappings are enabled: %d\n", LUIDMappingsEnabled);
    if (LUIDMappingsEnabled != 0)
    {
        CheckDirectorySecurityWithOwnerAndGroup(L"\\??", SeExports->SeAliasAdminsSid, NULL, // Group is "Domain Users"
                               4, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE |
                                                           OBJECT_INHERIT_ACE,      SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                                  ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE |
                                                           OBJECT_INHERIT_ACE,      SeExports->SeAliasAdminsSid, DIRECTORY_ALL_ACCESS,
                                  ACCESS_ALLOWED_ACE_TYPE, 0,                       SeExports->SeAliasAdminsSid, DIRECTORY_ALL_ACCESS,
                                  ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                           CONTAINER_INHERIT_ACE |
                                                           OBJECT_INHERIT_ACE,      SeExports->SeCreatorOwnerSid,GENERIC_ALL);
    }
    else
    {
        CheckDirectorySecurityWithOwnerAndGroup(L"\\??", SeExports->SeAliasAdminsSid, NULL, // Group is "Domain Users"
                               6, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid, READ_CONTROL | DIRECTORY_TRAVERSE | DIRECTORY_QUERY,
                                  ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                                  ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                           CONTAINER_INHERIT_ACE |
                                                           OBJECT_INHERIT_ACE,      SeExports->SeWorldSid, GENERIC_EXECUTE,
                                  ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                           CONTAINER_INHERIT_ACE |
                                                           OBJECT_INHERIT_ACE,      SeExports->SeAliasAdminsSid,GENERIC_ALL,
                                  ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                           CONTAINER_INHERIT_ACE |
                                                           OBJECT_INHERIT_ACE,      SeExports->SeLocalSystemSid,GENERIC_ALL,
                                  ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                           CONTAINER_INHERIT_ACE |
                                                           OBJECT_INHERIT_ACE,      SeExports->SeCreatorOwnerSid,GENERIC_ALL);
    }

    CheckDirectorySecurity(L"\\",
                           4, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid,       DIRECTORY_GENERIC_READ,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeRestrictedSid,  DIRECTORY_GENERIC_READ);

    CheckDirectorySecurity(L"\\ArcName",
                           4, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid,       DIRECTORY_GENERIC_READ,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeRestrictedSid,  DIRECTORY_GENERIC_READ);

    CheckDirectorySecurity(L"\\BaseNamedObjects",
                           3, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid,       DIRECTORY_GENERIC_WRITE | DIRECTORY_GENERIC_READ,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeRestrictedSid,  DIRECTORY_TRAVERSE);

    CheckDirectorySecurity(L"\\Callback",
                           3, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid,       DIRECTORY_GENERIC_READ,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid, DIRECTORY_ALL_ACCESS);

    CheckDirectorySecurity(L"\\Device",
                           4, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid,       DIRECTORY_GENERIC_READ,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeRestrictedSid,  DIRECTORY_GENERIC_READ);

    CheckDirectorySecurity(L"\\Driver",
                           2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid, DIRECTORY_GENERIC_READ);

    CheckDirectorySecurity(L"\\GLOBAL??",
                           6, ACCESS_ALLOWED_ACE_TYPE, 0,                       SeExports->SeWorldSid,       DIRECTORY_GENERIC_READ,
                              ACCESS_ALLOWED_ACE_TYPE, 0,                       SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                       CONTAINER_INHERIT_ACE |
                                                       OBJECT_INHERIT_ACE,      SeExports->SeWorldSid,       GENERIC_EXECUTE,
                              ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                       CONTAINER_INHERIT_ACE |
                                                       OBJECT_INHERIT_ACE,      SeExports->SeAliasAdminsSid, GENERIC_ALL,
                              ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                       CONTAINER_INHERIT_ACE |
                                                       OBJECT_INHERIT_ACE,      SeExports->SeLocalSystemSid, GENERIC_ALL,
                              ACCESS_ALLOWED_ACE_TYPE, INHERIT_ONLY_ACE |
                                                       CONTAINER_INHERIT_ACE |
                                                       OBJECT_INHERIT_ACE,      SeExports->SeCreatorOwnerSid,GENERIC_ALL);

    CheckDirectorySecurity(L"\\KernelObjects",
                           3, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeWorldSid,       DIRECTORY_GENERIC_READ,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS);

    CheckDirectorySecurity(L"\\ObjectTypes",
                           2, ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeLocalSystemSid, DIRECTORY_ALL_ACCESS,
                              ACCESS_ALLOWED_ACE_TYPE, 0, SeExports->SeAliasAdminsSid, DIRECTORY_GENERIC_READ);
}
