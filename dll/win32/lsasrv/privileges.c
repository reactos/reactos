/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/privileges.c
 * PURPOSE:         Privilege lookup functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "lsasrv.h"


typedef struct
{
    LUID Luid;
    LPCWSTR Name;
} PRIVILEGE_DATA;


static const PRIVILEGE_DATA WellKnownPrivileges[] =
{
    {{SE_CREATE_TOKEN_PRIVILEGE, 0}, SE_CREATE_TOKEN_NAME},
    {{SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, 0}, SE_ASSIGNPRIMARYTOKEN_NAME},
    {{SE_LOCK_MEMORY_PRIVILEGE, 0}, SE_LOCK_MEMORY_NAME},
    {{SE_INCREASE_QUOTA_PRIVILEGE, 0}, SE_INCREASE_QUOTA_NAME},
    {{SE_MACHINE_ACCOUNT_PRIVILEGE, 0}, SE_MACHINE_ACCOUNT_NAME},
    {{SE_TCB_PRIVILEGE, 0}, SE_TCB_NAME},
    {{SE_SECURITY_PRIVILEGE, 0}, SE_SECURITY_NAME},
    {{SE_TAKE_OWNERSHIP_PRIVILEGE, 0}, SE_TAKE_OWNERSHIP_NAME},
    {{SE_LOAD_DRIVER_PRIVILEGE, 0}, SE_LOAD_DRIVER_NAME},
    {{SE_SYSTEM_PROFILE_PRIVILEGE, 0}, SE_SYSTEM_PROFILE_NAME},
    {{SE_SYSTEMTIME_PRIVILEGE, 0}, SE_SYSTEMTIME_NAME},
    {{SE_PROF_SINGLE_PROCESS_PRIVILEGE, 0}, SE_PROF_SINGLE_PROCESS_NAME},
    {{SE_INC_BASE_PRIORITY_PRIVILEGE, 0}, SE_INC_BASE_PRIORITY_NAME},
    {{SE_CREATE_PAGEFILE_PRIVILEGE, 0}, SE_CREATE_PAGEFILE_NAME},
    {{SE_CREATE_PERMANENT_PRIVILEGE, 0}, SE_CREATE_PERMANENT_NAME},
    {{SE_BACKUP_PRIVILEGE, 0}, SE_BACKUP_NAME},
    {{SE_RESTORE_PRIVILEGE, 0}, SE_RESTORE_NAME},
    {{SE_SHUTDOWN_PRIVILEGE, 0}, SE_SHUTDOWN_NAME},
    {{SE_DEBUG_PRIVILEGE, 0}, SE_DEBUG_NAME},
    {{SE_AUDIT_PRIVILEGE, 0}, SE_AUDIT_NAME},
    {{SE_SYSTEM_ENVIRONMENT_PRIVILEGE, 0}, SE_SYSTEM_ENVIRONMENT_NAME},
    {{SE_CHANGE_NOTIFY_PRIVILEGE, 0}, SE_CHANGE_NOTIFY_NAME},
    {{SE_REMOTE_SHUTDOWN_PRIVILEGE, 0}, SE_REMOTE_SHUTDOWN_NAME},
    {{SE_UNDOCK_PRIVILEGE, 0}, SE_UNDOCK_NAME},
    {{SE_SYNC_AGENT_PRIVILEGE, 0}, SE_SYNC_AGENT_NAME},
    {{SE_ENABLE_DELEGATION_PRIVILEGE, 0}, SE_ENABLE_DELEGATION_NAME},
    {{SE_MANAGE_VOLUME_PRIVILEGE, 0}, SE_MANAGE_VOLUME_NAME},
    {{SE_IMPERSONATE_PRIVILEGE, 0}, SE_IMPERSONATE_NAME},
    {{SE_CREATE_GLOBAL_PRIVILEGE, 0}, SE_CREATE_GLOBAL_NAME}
};


/* FUNCTIONS ***************************************************************/

NTSTATUS
LsarpLookupPrivilegeName(PLUID Value,
                         PUNICODE_STRING *Name)
{
    PUNICODE_STRING NameBuffer;
    ULONG Priv;

    if (Value->HighPart != 0 ||
        (Value->LowPart < SE_MIN_WELL_KNOWN_PRIVILEGE ||
         Value->LowPart > SE_MAX_WELL_KNOWN_PRIVILEGE))
    {
        return STATUS_NO_SUCH_PRIVILEGE;
    }

    for (Priv = 0; Priv < sizeof(WellKnownPrivileges) / sizeof(WellKnownPrivileges[0]); Priv++)
    {
        if (Value->LowPart == WellKnownPrivileges[Priv].Luid.LowPart &&
            Value->HighPart == WellKnownPrivileges[Priv].Luid.HighPart)
        {
            NameBuffer = MIDL_user_allocate(sizeof(UNICODE_STRING));
            if (NameBuffer == NULL)
                return STATUS_NO_MEMORY;

            NameBuffer->Length = wcslen(WellKnownPrivileges[Priv].Name) * sizeof(WCHAR);
            NameBuffer->MaximumLength = NameBuffer->Length + sizeof(WCHAR);

            NameBuffer->Buffer = MIDL_user_allocate(NameBuffer->MaximumLength);
            if (NameBuffer == NULL)
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
LsarpLookupPrivilegeValue(PUNICODE_STRING Name,
                          PLUID Value)
{
    ULONG Priv;

    if (Name->Length == 0 || Name->Buffer == NULL)
        return STATUS_NO_SUCH_PRIVILEGE;

    for (Priv = 0; Priv < sizeof(WellKnownPrivileges) / sizeof(WellKnownPrivileges[0]); Priv++)
    {
        if (_wcsicmp(Name->Buffer, WellKnownPrivileges[Priv].Name) == 0)
        {
//            Value->LowPart = WellKnownPrivileges[Priv].Luid.LowPart;
//            Value->HighPart = WellKnownPrivileges[Priv].Luid.HighPart;
            *Value = WellKnownPrivileges[Priv].Luid;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_SUCH_PRIVILEGE;
}
