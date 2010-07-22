#define WIN32_NO_STATUS
#include <windows.h>
#include <ntsecapi.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <wchar.h>
#include <string.h>


#include <wine/debug.h>


typedef struct
{
    LUID Luid;
    LPCWSTR Name;
} PRIVILEGE_DATA;


static const PRIVILEGE_DATA WellKnownPrivileges[] =
{
    {{SE_CREATE_TOKEN_PRIVILEGE, 0}, L"SeCreateTokenPrivilege"},
    {{SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, 0}, L"SeAssignPrimaryTokenPrivilege"},
    {{SE_LOCK_MEMORY_PRIVILEGE, 0}, L"SeLockMemoryPrivilege"},
    {{SE_INCREASE_QUOTA_PRIVILEGE, 0}, L"SeIncreaseQuotaPrivilege"},
    {{SE_MACHINE_ACCOUNT_PRIVILEGE, 0}, L"SeMachineAccountPrivilege"},
    {{SE_TCB_PRIVILEGE, 0}, L"SeTcbPrivilege"},
    {{SE_SECURITY_PRIVILEGE, 0}, L"SeSecurityPrivilege"},
    {{SE_TAKE_OWNERSHIP_PRIVILEGE, 0}, L"SeTakeOwnershipPrivilege"},
    {{SE_LOAD_DRIVER_PRIVILEGE, 0}, L"SeLoadDriverPrivilege"},
    {{SE_SYSTEM_PROFILE_PRIVILEGE, 0}, L"SeSystemProfilePrivilege"},
    {{SE_SYSTEMTIME_PRIVILEGE, 0}, L"SeSystemtimePrivilege"},
    {{SE_PROF_SINGLE_PROCESS_PRIVILEGE, 0}, L"SeProfileSingleProcessPrivilege"},
    {{SE_INC_BASE_PRIORITY_PRIVILEGE, 0}, L"SeIncreaseBasePriorityPrivilege"},
    {{SE_CREATE_PAGEFILE_PRIVILEGE, 0}, L"SeCreatePagefilePrivilege"},
    {{SE_CREATE_PERMANENT_PRIVILEGE, 0}, L"SeCreatePermanentPrivilege"},
    {{SE_BACKUP_PRIVILEGE, 0}, L"SeBackupPrivilege"},
    {{SE_RESTORE_PRIVILEGE, 0}, L"SeRestorePrivilege"},
    {{SE_SHUTDOWN_PRIVILEGE, 0}, L"SeShutdownPrivilege"},
    {{SE_DEBUG_PRIVILEGE, 0}, L"SeDebugPrivilege"},
    {{SE_AUDIT_PRIVILEGE, 0}, L"SeAuditPrivilege"},
    {{SE_SYSTEM_ENVIRONMENT_PRIVILEGE, 0}, L"SeSystemEnvironmentPrivilege"},
    {{SE_CHANGE_NOTIFY_PRIVILEGE, 0}, L"SeChangeNotifyPrivilege"},
    {{SE_REMOTE_SHUTDOWN_PRIVILEGE, 0}, L"SeRemoteShutdownPrivilege"},
    {{SE_UNDOCK_PRIVILEGE, 0}, L"SeUndockPrivilege"},
    {{SE_SYNC_AGENT_PRIVILEGE, 0}, L"SeSyncAgentPrivilege"},
    {{SE_ENABLE_DELEGATION_PRIVILEGE, 0}, L"SeEnableDelegationPrivilege"},
    {{SE_MANAGE_VOLUME_PRIVILEGE, 0}, L"SeManageVolumePrivilege"},
    {{SE_IMPERSONATE_PRIVILEGE, 0}, L"SeImpersonatePrivilege"},
    {{SE_CREATE_GLOBAL_PRIVILEGE, 0}, L"SeCreateGlobalPrivilege"}
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
            Value->LowPart = WellKnownPrivileges[Priv].Luid.LowPart;
            Value->HighPart = WellKnownPrivileges[Priv].Luid.HighPart;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_SUCH_PRIVILEGE;
}
