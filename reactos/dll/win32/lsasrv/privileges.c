#define WIN32_NO_STATUS
#include <windows.h>
#include <ntsecapi.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <wchar.h>
#include <string.h>


#include <wine/debug.h>


static const WCHAR SE_CREATE_TOKEN_NAME_W[] =
 { 'S','e','C','r','e','a','t','e','T','o','k','e','n','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_ASSIGNPRIMARYTOKEN_NAME_W[] =
 { 'S','e','A','s','s','i','g','n','P','r','i','m','a','r','y','T','o','k','e','n','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_LOCK_MEMORY_NAME_W[] =
 { 'S','e','L','o','c','k','M','e','m','o','r','y','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_INCREASE_QUOTA_NAME_W[] =
 { 'S','e','I','n','c','r','e','a','s','e','Q','u','o','t','a','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_MACHINE_ACCOUNT_NAME_W[] =
 { 'S','e','M','a','c','h','i','n','e','A','c','c','o','u','n','t','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_TCB_NAME_W[] =
 { 'S','e','T','c','b','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_SECURITY_NAME_W[] =
 { 'S','e','S','e','c','u','r','i','t','y','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_TAKE_OWNERSHIP_NAME_W[] =
 { 'S','e','T','a','k','e','O','w','n','e','r','s','h','i','p','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_LOAD_DRIVER_NAME_W[] =
 { 'S','e','L','o','a','d','D','r','i','v','e','r','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_SYSTEM_PROFILE_NAME_W[] =
 { 'S','e','S','y','s','t','e','m','P','r','o','f','i','l','e','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_SYSTEMTIME_NAME_W[] =
 { 'S','e','S','y','s','t','e','m','t','i','m','e','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_PROF_SINGLE_PROCESS_NAME_W[] =
 { 'S','e','P','r','o','f','i','l','e','S','i','n','g','l','e','P','r','o','c','e','s','s','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_INC_BASE_PRIORITY_NAME_W[] =
 { 'S','e','I','n','c','r','e','a','s','e','B','a','s','e','P','r','i','o','r','i','t','y','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_CREATE_PAGEFILE_NAME_W[] =
 { 'S','e','C','r','e','a','t','e','P','a','g','e','f','i','l','e','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_CREATE_PERMANENT_NAME_W[] =
 { 'S','e','C','r','e','a','t','e','P','e','r','m','a','n','e','n','t','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_BACKUP_NAME_W[] =
 { 'S','e','B','a','c','k','u','p','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_RESTORE_NAME_W[] =
 { 'S','e','R','e','s','t','o','r','e','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_SHUTDOWN_NAME_W[] =
 { 'S','e','S','h','u','t','d','o','w','n','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_DEBUG_NAME_W[] =
 { 'S','e','D','e','b','u','g','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_AUDIT_NAME_W[] =
 { 'S','e','A','u','d','i','t','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_SYSTEM_ENVIRONMENT_NAME_W[] =
 { 'S','e','S','y','s','t','e','m','E','n','v','i','r','o','n','m','e','n','t','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_CHANGE_NOTIFY_NAME_W[] =
 { 'S','e','C','h','a','n','g','e','N','o','t','i','f','y','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_REMOTE_SHUTDOWN_NAME_W[] =
 { 'S','e','R','e','m','o','t','e','S','h','u','t','d','o','w','n','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_UNDOCK_NAME_W[] =
 { 'S','e','U','n','d','o','c','k','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_SYNC_AGENT_NAME_W[] =
 { 'S','e','S','y','n','c','A','g','e','n','t','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_ENABLE_DELEGATION_NAME_W[] =
 { 'S','e','E','n','a','b','l','e','D','e','l','e','g','a','t','i','o','n','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_MANAGE_VOLUME_NAME_W[] =
 { 'S','e','M','a','n','a','g','e','V','o','l','u','m','e','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_IMPERSONATE_NAME_W[] =
 { 'S','e','I','m','p','e','r','s','o','n','a','t','e','P','r','i','v','i','l','e','g','e',0 };
static const WCHAR SE_CREATE_GLOBAL_NAME_W[] =
 { 'S','e','C','r','e','a','t','e','G','l','o','b','a','l','P','r','i','v','i','l','e','g','e',0 };

static const WCHAR * const WellKnownPrivNames[SE_MAX_WELL_KNOWN_PRIVILEGE + 1] =
{
    NULL,
    NULL,
    SE_CREATE_TOKEN_NAME_W,
    SE_ASSIGNPRIMARYTOKEN_NAME_W,
    SE_LOCK_MEMORY_NAME_W,
    SE_INCREASE_QUOTA_NAME_W,
    SE_MACHINE_ACCOUNT_NAME_W,
    SE_TCB_NAME_W,
    SE_SECURITY_NAME_W,
    SE_TAKE_OWNERSHIP_NAME_W,
    SE_LOAD_DRIVER_NAME_W,
    SE_SYSTEM_PROFILE_NAME_W,
    SE_SYSTEMTIME_NAME_W,
    SE_PROF_SINGLE_PROCESS_NAME_W,
    SE_INC_BASE_PRIORITY_NAME_W,
    SE_CREATE_PAGEFILE_NAME_W,
    SE_CREATE_PERMANENT_NAME_W,
    SE_BACKUP_NAME_W,
    SE_RESTORE_NAME_W,
    SE_SHUTDOWN_NAME_W,
    SE_DEBUG_NAME_W,
    SE_AUDIT_NAME_W,
    SE_SYSTEM_ENVIRONMENT_NAME_W,
    SE_CHANGE_NOTIFY_NAME_W,
    SE_REMOTE_SHUTDOWN_NAME_W,
    SE_UNDOCK_NAME_W,
    SE_SYNC_AGENT_NAME_W,
    SE_ENABLE_DELEGATION_NAME_W,
    SE_MANAGE_VOLUME_NAME_W,
    SE_IMPERSONATE_NAME_W,
    SE_CREATE_GLOBAL_NAME_W,
};

static const WCHAR * const DefaultPrivNames[] =
{
    L"SeCreateTokenPrivilege",
    L"SeAssignPrimaryTokenPrivilege",
    L"SeLockMemoryPrivilege",
    L"SeIncreaseQuotaPrivilege",
    L"SeMachineAccountPrivilege",
    L"SeTcbPrivilege",
    L"SeSecurityPrivilege",
    L"SeTakeOwnershipPrivilege",
    L"SeLoadDriverPrivilege",
    L"SeSystemProfilePrivilege",
    L"SeSystemtimePrivilege",
    L"SeProfileSingleProcessPrivilege",
    L"SeIncreaseBasePriorityPrivilege",
    L"SeCreatePagefilePrivilege",
    L"SeCreatePermanentPrivilege",
    L"SeBackupPrivilege",
    L"SeRestorePrivilege",
    L"SeShutdownPrivilege",
    L"SeDebugPrivilege",
    L"SeAuditPrivilege",
    L"SeSystemEnvironmentPrivilege",
    L"SeChangeNotifyPrivilege",
    L"SeRemoteShutdownPrivilege",
    L"SeUndockPrivilege",
    L"SeSyncAgentPrivilege",
    L"SeEnableDelegationPrivilege",
    L"SeManageVolumePrivilege",
    L"SeImpersonatePrivilege",
    L"SeCreateGlobalPrivilege"
};


NTSTATUS
LsarpLookupPrivilegeName(PLUID Value,
                         PUNICODE_STRING *Name)
{
    PUNICODE_STRING NameBuffer;

    if (Value->HighPart ||
        (Value->LowPart < SE_MIN_WELL_KNOWN_PRIVILEGE ||
         Value->LowPart > SE_MAX_WELL_KNOWN_PRIVILEGE))
    {
        return STATUS_NO_SUCH_PRIVILEGE;
    }

    NameBuffer = MIDL_user_allocate(sizeof(UNICODE_STRING));
    if (NameBuffer == NULL)
        return STATUS_NO_MEMORY;

    NameBuffer->Length = wcslen(WellKnownPrivNames[Value->LowPart]) * sizeof(WCHAR);
    NameBuffer->MaximumLength = NameBuffer->Length + sizeof(WCHAR);

    NameBuffer->Buffer = MIDL_user_allocate(NameBuffer->MaximumLength);
    if (NameBuffer == NULL)
    {
        MIDL_user_free(NameBuffer);
        return STATUS_NO_MEMORY;
    }

    wcscpy(NameBuffer->Buffer, WellKnownPrivNames[Value->LowPart]);

    *Name = NameBuffer;

    return STATUS_SUCCESS;
}


NTSTATUS
LsarpLookupPrivilegeValue(PUNICODE_STRING Name,
                          PLUID Value)
{
    ULONG Priv;

    if (Name->Length == 0 || Name->Buffer == NULL)
        return STATUS_NO_SUCH_PRIVILEGE;

    for (Priv = 0; Priv < sizeof(DefaultPrivNames) / sizeof(DefaultPrivNames[0]); Priv++)
    {
        if (0 == _wcsicmp(Name->Buffer, DefaultPrivNames[Priv]))
        {
            Value->LowPart = Priv + SE_MIN_WELL_KNOWN_PRIVILEGE;
            Value->HighPart = 0;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_SUCH_PRIVILEGE;
}
