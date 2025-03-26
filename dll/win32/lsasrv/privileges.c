/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/privileges.c
 * PURPOSE:         Privilege lookup functions
 *
 * PROGRAMMERS:     Eric Kohl <eric.kohl@reactos.org>
 */

#include "lsasrv.h"
#include "resources.h"

typedef struct
{
    LUID Luid;
    LPCWSTR Name;
    INT DisplayNameId;
} PRIVILEGE_DATA;

typedef struct
{
    ULONG Flag;
    LPCWSTR Name;
} RIGHT_DATA;


/* GLOBALS *****************************************************************/

static const PRIVILEGE_DATA WellKnownPrivileges[] =
{
    {{SE_CREATE_TOKEN_PRIVILEGE, 0}, SE_CREATE_TOKEN_NAME, IDS_CREATE_TOKEN_PRIVILEGE},
    {{SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, 0}, SE_ASSIGNPRIMARYTOKEN_NAME, IDS_ASSIGNPRIMARYTOKEN_PRIVILEGE},
    {{SE_LOCK_MEMORY_PRIVILEGE, 0}, SE_LOCK_MEMORY_NAME, IDS_LOCK_MEMORY_PRIVILEGE},
    {{SE_INCREASE_QUOTA_PRIVILEGE, 0}, SE_INCREASE_QUOTA_NAME, IDS_INCREASE_QUOTA_PRIVILEGE},
    {{SE_MACHINE_ACCOUNT_PRIVILEGE, 0}, SE_MACHINE_ACCOUNT_NAME, IDS_MACHINE_ACCOUNT_PRIVILEGE},
    {{SE_TCB_PRIVILEGE, 0}, SE_TCB_NAME, IDS_TCB_PRIVILEGE},
    {{SE_SECURITY_PRIVILEGE, 0}, SE_SECURITY_NAME, IDS_SECURITY_PRIVILEGE},
    {{SE_TAKE_OWNERSHIP_PRIVILEGE, 0}, SE_TAKE_OWNERSHIP_NAME, IDS_TAKE_OWNERSHIP_PRIVILEGE},
    {{SE_LOAD_DRIVER_PRIVILEGE, 0}, SE_LOAD_DRIVER_NAME, IDS_LOAD_DRIVER_PRIVILEGE},
    {{SE_SYSTEM_PROFILE_PRIVILEGE, 0}, SE_SYSTEM_PROFILE_NAME, IDS_SYSTEM_PROFILE_PRIVILEGE},
    {{SE_SYSTEMTIME_PRIVILEGE, 0}, SE_SYSTEMTIME_NAME, IDS_SYSTEMTIME_PRIVILEGE},
    {{SE_PROF_SINGLE_PROCESS_PRIVILEGE, 0}, SE_PROF_SINGLE_PROCESS_NAME, IDS_PROF_SINGLE_PROCESS_PRIVILEGE},
    {{SE_INC_BASE_PRIORITY_PRIVILEGE, 0}, SE_INC_BASE_PRIORITY_NAME, IDS_INC_BASE_PRIORITY_PRIVILEGE},
    {{SE_CREATE_PAGEFILE_PRIVILEGE, 0}, SE_CREATE_PAGEFILE_NAME, IDS_CREATE_PAGEFILE_PRIVILEGE},
    {{SE_CREATE_PERMANENT_PRIVILEGE, 0}, SE_CREATE_PERMANENT_NAME, IDS_CREATE_PERMANENT_PRIVILEGE},
    {{SE_BACKUP_PRIVILEGE, 0}, SE_BACKUP_NAME, IDS_BACKUP_PRIVILEGE},
    {{SE_RESTORE_PRIVILEGE, 0}, SE_RESTORE_NAME, IDS_RESTORE_PRIVILEGE},
    {{SE_SHUTDOWN_PRIVILEGE, 0}, SE_SHUTDOWN_NAME, IDS_SHUTDOWN_PRIVILEGE},
    {{SE_DEBUG_PRIVILEGE, 0}, SE_DEBUG_NAME, IDS_DEBUG_PRIVILEGE},
    {{SE_AUDIT_PRIVILEGE, 0}, SE_AUDIT_NAME, IDS_AUDIT_PRIVILEGE},
    {{SE_SYSTEM_ENVIRONMENT_PRIVILEGE, 0}, SE_SYSTEM_ENVIRONMENT_NAME, IDS_SYSTEM_ENVIRONMENT_PRIVILEGE},
    {{SE_CHANGE_NOTIFY_PRIVILEGE, 0}, SE_CHANGE_NOTIFY_NAME, IDS_CHANGE_NOTIFY_PRIVILEGE},
    {{SE_REMOTE_SHUTDOWN_PRIVILEGE, 0}, SE_REMOTE_SHUTDOWN_NAME, IDS_REMOTE_SHUTDOWN_PRIVILEGE},
    {{SE_UNDOCK_PRIVILEGE, 0}, SE_UNDOCK_NAME, IDS_UNDOCK_PRIVILEGE},
    {{SE_SYNC_AGENT_PRIVILEGE, 0}, SE_SYNC_AGENT_NAME, IDS_SYNC_AGENT_PRIVILEGE},
    {{SE_ENABLE_DELEGATION_PRIVILEGE, 0}, SE_ENABLE_DELEGATION_NAME, IDS_ENABLE_DELEGATION_PRIVILEGE},
    {{SE_MANAGE_VOLUME_PRIVILEGE, 0}, SE_MANAGE_VOLUME_NAME, IDS_MANAGE_VOLUME_PRIVILEGE},
    {{SE_IMPERSONATE_PRIVILEGE, 0}, SE_IMPERSONATE_NAME, IDS_IMPERSONATE_PRIVILEGE},
    {{SE_CREATE_GLOBAL_PRIVILEGE, 0}, SE_CREATE_GLOBAL_NAME, IDS_CREATE_GLOBAL_PRIVILEGE}
};

static const RIGHT_DATA WellKnownRights[] =
{
    {SECURITY_ACCESS_INTERACTIVE_LOGON, SE_INTERACTIVE_LOGON_NAME},
    {SECURITY_ACCESS_NETWORK_LOGON, SE_NETWORK_LOGON_NAME},
    {SECURITY_ACCESS_BATCH_LOGON, SE_BATCH_LOGON_NAME},
    {SECURITY_ACCESS_SERVICE_LOGON, SE_SERVICE_LOGON_NAME},
    {SECURITY_ACCESS_DENY_INTERACTIVE_LOGON, SE_DENY_INTERACTIVE_LOGON_NAME},
    {SECURITY_ACCESS_DENY_NETWORK_LOGON, SE_DENY_NETWORK_LOGON_NAME},
    {SECURITY_ACCESS_DENY_BATCH_LOGON, SE_DENY_BATCH_LOGON_NAME},
    {SECURITY_ACCESS_DENY_SERVICE_LOGON, SE_DENY_SERVICE_LOGON_NAME},
    {SECURITY_ACCESS_REMOTE_INTERACTIVE_LOGON, SE_REMOTE_INTERACTIVE_LOGON_NAME},
    {SECURITY_ACCESS_DENY_REMOTE_INTERACTIVE_LOGON, SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME}
};


/* FUNCTIONS ***************************************************************/

NTSTATUS
LsarpLookupPrivilegeName(PLUID Value,
                         PRPC_UNICODE_STRING *Name)
{
    PRPC_UNICODE_STRING NameBuffer;
    ULONG Priv;

    if (Value->HighPart != 0 ||
        (Value->LowPart < SE_MIN_WELL_KNOWN_PRIVILEGE ||
         Value->LowPart > SE_MAX_WELL_KNOWN_PRIVILEGE))
    {
        return STATUS_NO_SUCH_PRIVILEGE;
    }

    for (Priv = 0; Priv < ARRAYSIZE(WellKnownPrivileges); Priv++)
    {
        if (Value->LowPart == WellKnownPrivileges[Priv].Luid.LowPart &&
            Value->HighPart == WellKnownPrivileges[Priv].Luid.HighPart)
        {
            NameBuffer = MIDL_user_allocate(sizeof(RPC_UNICODE_STRING));
            if (NameBuffer == NULL)
                return STATUS_NO_MEMORY;

            NameBuffer->Length = (USHORT)wcslen(WellKnownPrivileges[Priv].Name) * sizeof(WCHAR);
            NameBuffer->MaximumLength = NameBuffer->Length + sizeof(WCHAR);

            NameBuffer->Buffer = MIDL_user_allocate(NameBuffer->MaximumLength);
            if (NameBuffer->Buffer == NULL)
            {
                MIDL_user_free(NameBuffer);
                return STATUS_NO_MEMORY;
            }

            wcscpy(NameBuffer->Buffer, WellKnownPrivileges[Priv].Name);

            *Name = NameBuffer;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_SUCH_PRIVILEGE;
}


NTSTATUS
LsarpLookupPrivilegeDisplayName(PRPC_UNICODE_STRING Name,
                                USHORT ClientLanguage,
                                USHORT ClientSystemDefaultLanguage,
                                PRPC_UNICODE_STRING *DisplayName,
                                USHORT *LanguageReturned)
{
    PRPC_UNICODE_STRING DisplayNameBuffer;
    HINSTANCE hInstance;
    ULONG Index;
    UINT nLength;

    TRACE("LsarpLookupPrivilegeDisplayName(%p 0x%04hu 0x%04hu %p %p)\n",
          Name, ClientLanguage, ClientSystemDefaultLanguage, DisplayName, LanguageReturned);

    if (Name->Length == 0 || Name->Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    hInstance = GetModuleHandleW(L"lsasrv.dll");

    for (Index = 0; Index < ARRAYSIZE(WellKnownPrivileges); Index++)
    {
        if (_wcsicmp(Name->Buffer, WellKnownPrivileges[Index].Name) == 0)
        {
            TRACE("Index: %u\n", Index);
            nLength = LsapGetResourceStringLengthEx(hInstance,
                                                    IDS_CREATE_TOKEN_PRIVILEGE + Index,
                                                    ClientLanguage);
            if (nLength != 0)
            {
                DisplayNameBuffer = MIDL_user_allocate(sizeof(RPC_UNICODE_STRING));
                if (DisplayNameBuffer == NULL)
                    return STATUS_NO_MEMORY;

                DisplayNameBuffer->Length = nLength * sizeof(WCHAR);
                DisplayNameBuffer->MaximumLength = DisplayNameBuffer->Length + sizeof(WCHAR);

                DisplayNameBuffer->Buffer = MIDL_user_allocate(DisplayNameBuffer->MaximumLength);
                if (DisplayNameBuffer->Buffer == NULL)
                {
                    MIDL_user_free(DisplayNameBuffer);
                    return STATUS_NO_MEMORY;
                }

                LsapLoadStringEx(hInstance,
                                 IDS_CREATE_TOKEN_PRIVILEGE + Index,
                                 ClientLanguage,
                                 DisplayNameBuffer->Buffer,
                                 nLength);

                *DisplayName = DisplayNameBuffer;
                *LanguageReturned = ClientLanguage;
            }
            else
            {
                nLength = LsapGetResourceStringLengthEx(hInstance,
                                                        IDS_CREATE_TOKEN_PRIVILEGE + Index,
                                                        ClientSystemDefaultLanguage);
                if (nLength != 0)
                {
                    DisplayNameBuffer = MIDL_user_allocate(sizeof(RPC_UNICODE_STRING));
                    if (DisplayNameBuffer == NULL)
                        return STATUS_NO_MEMORY;

                    DisplayNameBuffer->Length = nLength * sizeof(WCHAR);
                    DisplayNameBuffer->MaximumLength = DisplayNameBuffer->Length + sizeof(WCHAR);

                    DisplayNameBuffer->Buffer = MIDL_user_allocate(DisplayNameBuffer->MaximumLength);
                    if (DisplayNameBuffer->Buffer == NULL)
                    {
                        MIDL_user_free(DisplayNameBuffer);
                        return STATUS_NO_MEMORY;
                    }

                    LsapLoadStringEx(hInstance,
                                     IDS_CREATE_TOKEN_PRIVILEGE + Index,
                                     ClientSystemDefaultLanguage,
                                     DisplayNameBuffer->Buffer,
                                     nLength);

                    *DisplayName = DisplayNameBuffer;
                    *LanguageReturned = ClientSystemDefaultLanguage;
                }
                else
                {
                    return STATUS_INVALID_PARAMETER;
#if 0
                    nLength = LsapGetResourceStringLengthEx(hInstance,
                                                            IDS_CREATE_TOKEN_PRIVILEGE + Index,
                                                            0x409);
#endif
                }
            }

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_SUCH_PRIVILEGE;
}


PLUID
LsarpLookupPrivilegeValue(
    IN PRPC_UNICODE_STRING Name)
{
    ULONG Priv;

    if (Name->Length == 0 || Name->Buffer == NULL)
        return NULL;

    for (Priv = 0; Priv < ARRAYSIZE(WellKnownPrivileges); Priv++)
    {
        if (_wcsicmp(Name->Buffer, WellKnownPrivileges[Priv].Name) == 0)
            return (PLUID)&(WellKnownPrivileges[Priv].Luid);
    }

    return NULL;
}


NTSTATUS
LsarpEnumeratePrivileges(DWORD *EnumerationContext,
                         PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
                         DWORD PreferedMaximumLength)
{
    PLSAPR_POLICY_PRIVILEGE_DEF Privileges = NULL;
    ULONG EnumIndex;
    ULONG EnumCount = 0;
    ULONG RequiredLength = 0;
    ULONG i;
    BOOLEAN MoreEntries = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;

    EnumIndex = *EnumerationContext;

    for (; EnumIndex < ARRAYSIZE(WellKnownPrivileges); EnumIndex++)
    {
        TRACE("EnumIndex: %lu\n", EnumIndex);
        TRACE("Privilege Name: %S\n", WellKnownPrivileges[EnumIndex].Name);
        TRACE("Name Length: %lu\n", wcslen(WellKnownPrivileges[EnumIndex].Name));

        if ((RequiredLength +
             wcslen(WellKnownPrivileges[EnumIndex].Name) * sizeof(WCHAR) +
             sizeof(UNICODE_NULL) +
             sizeof(LSAPR_POLICY_PRIVILEGE_DEF)) > PreferedMaximumLength)
        {
            MoreEntries = TRUE;
            break;
        }

        RequiredLength += (wcslen(WellKnownPrivileges[EnumIndex].Name) * sizeof(WCHAR) +
                           sizeof(UNICODE_NULL) + sizeof(LSAPR_POLICY_PRIVILEGE_DEF));
        EnumCount++;
    }

    TRACE("EnumCount: %lu\n", EnumCount);
    TRACE("RequiredLength: %lu\n", RequiredLength);

    if (EnumCount == 0)
        goto done;

    Privileges = MIDL_user_allocate(EnumCount * sizeof(LSAPR_POLICY_PRIVILEGE_DEF));
    if (Privileges == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    EnumIndex = *EnumerationContext;

    for (i = 0; i < EnumCount; i++, EnumIndex++)
    {
        Privileges[i].LocalValue = WellKnownPrivileges[EnumIndex].Luid;

        Privileges[i].Name.Length = (USHORT)wcslen(WellKnownPrivileges[EnumIndex].Name) * sizeof(WCHAR);
        Privileges[i].Name.MaximumLength = (USHORT)Privileges[i].Name.Length + sizeof(UNICODE_NULL);

        Privileges[i].Name.Buffer = MIDL_user_allocate(Privileges[i].Name.MaximumLength);
        if (Privileges[i].Name.Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        memcpy(Privileges[i].Name.Buffer,
               WellKnownPrivileges[EnumIndex].Name,
               Privileges[i].Name.Length);
    }

done:
    if (NT_SUCCESS(Status))
    {
        EnumerationBuffer->Entries = EnumCount;
        EnumerationBuffer->Privileges = Privileges;
        *EnumerationContext += EnumCount;
    }
    else
    {
        if (Privileges != NULL)
        {
            for (i = 0; i < EnumCount; i++)
            {
                if (Privileges[i].Name.Buffer != NULL)
                    MIDL_user_free(Privileges[i].Name.Buffer);
            }

            MIDL_user_free(Privileges);
        }
    }

    if ((Status == STATUS_SUCCESS) && (MoreEntries != FALSE))
        Status = STATUS_MORE_ENTRIES;

    return Status;
}


NTSTATUS
LsapLookupAccountRightName(ULONG RightValue,
                           PRPC_UNICODE_STRING *Name)
{
    PRPC_UNICODE_STRING NameBuffer;
    ULONG i;

    for (i = 0; i < ARRAYSIZE(WellKnownRights); i++)
    {
        if (WellKnownRights[i].Flag == RightValue)
        {
            NameBuffer = MIDL_user_allocate(sizeof(RPC_UNICODE_STRING));
            if (NameBuffer == NULL)
                return STATUS_NO_MEMORY;

            NameBuffer->Length = (USHORT)wcslen(WellKnownRights[i].Name) * sizeof(WCHAR);
            NameBuffer->MaximumLength = NameBuffer->Length + sizeof(WCHAR);

            NameBuffer->Buffer = MIDL_user_allocate(NameBuffer->MaximumLength);
            if (NameBuffer->Buffer == NULL)
            {
                MIDL_user_free(NameBuffer);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            wcscpy(NameBuffer->Buffer, WellKnownRights[i].Name);

            *Name = NameBuffer;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_SUCH_PRIVILEGE;
}


ACCESS_MASK
LsapLookupAccountRightValue(
    IN PRPC_UNICODE_STRING Name)
{
    ULONG i;

    if (Name->Length == 0 || Name->Buffer == NULL)
        return 0;

    for (i = 0; i < ARRAYSIZE(WellKnownRights); i++)
    {
        if (_wcsicmp(Name->Buffer, WellKnownRights[i].Name) == 0)
            return WellKnownRights[i].Flag;
    }

    return 0;
}

/* EOF */
