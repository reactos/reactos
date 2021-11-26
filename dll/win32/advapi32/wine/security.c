/*
 * COPYRIGHT:       See COPYING in the top level directory
 * WINE COPYRIGHT:
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
 * Copyright 2006 Robert Reif
 * Copyright 2006 Hervé Poussineau
 *
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/advapi32/wine/security.c
 */

#include <advapi32.h>

#include <sddl.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

static BOOL ParseStringSidToSid(LPCWSTR StringSid, PSID pSid, LPDWORD cBytes);
#ifdef __REACTOS__
VOID WINAPI QuerySecurityAccessMask(SECURITY_INFORMATION,LPDWORD);
VOID WINAPI SetSecurityAccessMask(SECURITY_INFORMATION,LPDWORD);
#endif

typedef struct _ACEFLAG
{
   LPCWSTR wstr;
   DWORD value;
} ACEFLAG, *LPACEFLAG;

typedef struct _MAX_SID
{
    /* same fields as struct _SID */
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[SID_MAX_SUB_AUTHORITIES];
} MAX_SID;

typedef struct WELLKNOWNSID
{
    WCHAR wstr[2];
    WELL_KNOWN_SID_TYPE Type;
    MAX_SID Sid;
} WELLKNOWNSID;

static const WELLKNOWNSID WellKnownSids[] =
{
    { {0,0}, WinNullSid, { SID_REVISION, 1, { SECURITY_NULL_SID_AUTHORITY }, { SECURITY_NULL_RID } } },
    { {'W','D'}, WinWorldSid, { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY }, { SECURITY_WORLD_RID } } },
    { {0,0}, WinLocalSid, { SID_REVISION, 1, { SECURITY_LOCAL_SID_AUTHORITY }, { SECURITY_LOCAL_RID } } },
    { {'C','O'}, WinCreatorOwnerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RID } } },
    { {'C','G'}, WinCreatorGroupSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_RID } } },
    { {0,0}, WinCreatorOwnerServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_SERVER_RID } } },
    { {0,0}, WinCreatorGroupServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_SERVER_RID } } },
    { {0,0}, WinNtAuthoritySid, { SID_REVISION, 0, { SECURITY_NT_AUTHORITY }, { SECURITY_NULL_RID } } },
    { {0,0}, WinDialupSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_DIALUP_RID } } },
    { {'N','U'}, WinNetworkSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_RID } } },
    { {0,0}, WinBatchSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BATCH_RID } } },
    { {'I','U'}, WinInteractiveSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_INTERACTIVE_RID } } },
    { {'S','U'}, WinServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_SERVICE_RID } } },
    { {'A','N'}, WinAnonymousSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ANONYMOUS_LOGON_RID } } },
    { {0,0}, WinProxySid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PROXY_RID } } },
    { {'E','D'}, WinEnterpriseControllersSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ENTERPRISE_CONTROLLERS_RID } } },
    { {'P','S'}, WinSelfSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PRINCIPAL_SELF_RID } } },
    { {'A','U'}, WinAuthenticatedUserSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_AUTHENTICATED_USER_RID } } },
    { {'R','C'}, WinRestrictedCodeSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_RESTRICTED_CODE_RID } } },
    { {0,0}, WinTerminalServerSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_TERMINAL_SERVER_RID } } },
    { {0,0}, WinRemoteLogonIdSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_REMOTE_LOGON_RID } } },
    { {'S','Y'}, WinLocalSystemSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } } },
    { {'L','S'}, WinLocalServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SERVICE_RID } } },
    { {'N','S'}, WinNetworkServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_SERVICE_RID } } },
    { {0,0}, WinBuiltinDomainSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID } } },
    { {'B','A'}, WinBuiltinAdministratorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS } } },
    { {'B','U'}, WinBuiltinUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS } } },
    { {'B','G'}, WinBuiltinGuestsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS } } },
    { {'P','U'}, WinBuiltinPowerUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS } } },
    { {'A','O'}, WinBuiltinAccountOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ACCOUNT_OPS } } },
    { {'S','O'}, WinBuiltinSystemOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS } } },
    { {'P','O'}, WinBuiltinPrintOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PRINT_OPS } } },
    { {'B','O'}, WinBuiltinBackupOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_BACKUP_OPS } } },
    { {'R','E'}, WinBuiltinReplicatorSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REPLICATOR } } },
    { {'R','U'}, WinBuiltinPreWindows2000CompatibleAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PREW2KCOMPACCESS } } },
    { {'R','D'}, WinBuiltinRemoteDesktopUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS } } },
    { {'N','O'}, WinBuiltinNetworkConfigurationOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS } } },
    { {0,0}, WinNTLMAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_NTLM_RID } } },
    { {0,0}, WinDigestAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_DIGEST_RID } } },
    { {0,0}, WinSChannelAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_SCHANNEL_RID } } },
    { {0,0}, WinThisOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_THIS_ORGANIZATION_RID } } },
    { {0,0}, WinOtherOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_OTHER_ORGANIZATION_RID } } },
    { {0,0}, WinBuiltinIncomingForestTrustBuildersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS  } } },
    { {0,0}, WinBuiltinPerfMonitoringUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_MONITORING_USERS } } },
    { {0,0}, WinBuiltinPerfLoggingUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_LOGGING_USERS } } },
    { {0,0}, WinBuiltinAuthorizationAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS } } },
    { {0,0}, WinBuiltinTerminalServerLicenseServersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS } } },
    { {0,0}, WinBuiltinDCOMUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_DCOM_USERS } } },
    { {'L','W'}, WinLowLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_LOW_RID} } },
    { {'M','E'}, WinMediumLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_MEDIUM_RID } } },
    { {'H','I'}, WinHighLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_HIGH_RID } } },
    { {'S','I'}, WinSystemLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_SYSTEM_RID } } },
};

/* these SIDs must be constructed as relative to some domain - only the RID is well-known */
typedef struct WELLKNOWNRID
{
    WCHAR wstr[2];
    WELL_KNOWN_SID_TYPE Type;
    DWORD Rid;
} WELLKNOWNRID;

static const WELLKNOWNRID WellKnownRids[] = {
    { {'L','A'}, WinAccountAdministratorSid,    DOMAIN_USER_RID_ADMIN },
    { {'L','G'}, WinAccountGuestSid,            DOMAIN_USER_RID_GUEST },
    { {0,0}, WinAccountKrbtgtSid,           DOMAIN_USER_RID_KRBTGT },
    { {'D','A'}, WinAccountDomainAdminsSid,     DOMAIN_GROUP_RID_ADMINS },
    { {'D','U'}, WinAccountDomainUsersSid,      DOMAIN_GROUP_RID_USERS },
    { {'D','G'}, WinAccountDomainGuestsSid,     DOMAIN_GROUP_RID_GUESTS },
    { {'D','C'}, WinAccountComputersSid,        DOMAIN_GROUP_RID_COMPUTERS },
    { {'D','D'}, WinAccountControllersSid,      DOMAIN_GROUP_RID_CONTROLLERS },
    { {'C','A'}, WinAccountCertAdminsSid,       DOMAIN_GROUP_RID_CERT_ADMINS },
    { {'S','A'}, WinAccountSchemaAdminsSid,     DOMAIN_GROUP_RID_SCHEMA_ADMINS },
    { {'E','A'}, WinAccountEnterpriseAdminsSid, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS },
    { {'P','A'}, WinAccountPolicyAdminsSid,     DOMAIN_GROUP_RID_POLICY_ADMINS },
    { {'R','S'}, WinAccountRasAndIasServersSid, DOMAIN_ALIAS_RID_RAS_SERVERS },
};

#ifndef __REACTOS__
static const SID sidWorld = { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY} , { SECURITY_WORLD_RID } };
#endif

static const WCHAR SDDL_NO_READ_UP[]       = {'N','R',0};
static const WCHAR SDDL_NO_WRITE_UP[]      = {'N','W',0};
static const WCHAR SDDL_NO_EXECUTE_UP[]    = {'N','X',0};

/*
 * ACE types
 */
static const WCHAR SDDL_ACCESS_ALLOWED[]        = {'A',0};
static const WCHAR SDDL_ACCESS_DENIED[]         = {'D',0};
#ifndef __REACTOS__
static const WCHAR SDDL_OBJECT_ACCESS_ALLOWED[] = {'O','A',0};
static const WCHAR SDDL_OBJECT_ACCESS_DENIED[]  = {'O','D',0};
#endif
static const WCHAR SDDL_AUDIT[]                 = {'A','U',0};
static const WCHAR SDDL_ALARM[]                 = {'A','L',0};
static const WCHAR SDDL_MANDATORY_LABEL[]       = {'M','L',0};
#ifndef __REACTOS__
static const WCHAR SDDL_OBJECT_AUDIT[]          = {'O','U',0};
static const WCHAR SDDL_OBJECT_ALARM[]          = {'O','L',0};
#endif

/*
 * SDDL ADS Rights
 */
#define ADS_RIGHT_DS_CREATE_CHILD   0x0001
#define ADS_RIGHT_DS_DELETE_CHILD   0x0002
#define ADS_RIGHT_ACTRL_DS_LIST     0x0004
#define ADS_RIGHT_DS_SELF           0x0008
#define ADS_RIGHT_DS_READ_PROP      0x0010
#define ADS_RIGHT_DS_WRITE_PROP     0x0020
#define ADS_RIGHT_DS_DELETE_TREE    0x0040
#define ADS_RIGHT_DS_LIST_OBJECT    0x0080
#define ADS_RIGHT_DS_CONTROL_ACCESS 0x0100

/*
 * ACE flags
 */
static const WCHAR SDDL_CONTAINER_INHERIT[]  = {'C','I',0};
static const WCHAR SDDL_OBJECT_INHERIT[]     = {'O','I',0};
static const WCHAR SDDL_NO_PROPAGATE[]       = {'N','P',0};
static const WCHAR SDDL_INHERIT_ONLY[]       = {'I','O',0};
static const WCHAR SDDL_INHERITED[]          = {'I','D',0};
static const WCHAR SDDL_AUDIT_SUCCESS[]      = {'S','A',0};
static const WCHAR SDDL_AUDIT_FAILURE[]      = {'F','A',0};

static const char * debugstr_sid(PSID sid)
{
    int auth = 0;
    SID * psid = (SID *)sid;

    if (psid == NULL)
        return "(null)";

    auth = psid->IdentifierAuthority.Value[5] +
           (psid->IdentifierAuthority.Value[4] << 8) +
           (psid->IdentifierAuthority.Value[3] << 16) +
           (psid->IdentifierAuthority.Value[2] << 24);

    switch (psid->SubAuthorityCount) {
    case 0:
        return wine_dbg_sprintf("S-%d-%d", psid->Revision, auth);
    case 1:
        return wine_dbg_sprintf("S-%d-%d-%lu", psid->Revision, auth,
            psid->SubAuthority[0]);
    case 2:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1]);
    case 3:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2]);
    case 4:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3]);
    case 5:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4]);
    case 6:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[3], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[0], psid->SubAuthority[4], psid->SubAuthority[5]);
    case 7:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
            psid->SubAuthority[6]);
    case 8:
        return wine_dbg_sprintf("S-%d-%d-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu", psid->Revision, auth,
            psid->SubAuthority[0], psid->SubAuthority[1], psid->SubAuthority[2],
            psid->SubAuthority[3], psid->SubAuthority[4], psid->SubAuthority[5],
            psid->SubAuthority[6], psid->SubAuthority[7]);
    }
    return "(too-big)";
}

/* set last error code from NT status and get the proper boolean return value */
/* used for functions that are a simple wrapper around the corresponding ntdll API */
static __inline BOOL set_ntstatus( NTSTATUS status )
{
    if (!NT_SUCCESS(status)) SetLastError( RtlNtStatusToDosError( status ));
    return NT_SUCCESS(status);
}

static LPWSTR SERV_dup( LPCSTR str )
{
    UINT len;
    LPWSTR wstr;

    if( !str )
        return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    wstr = heap_alloc( len*sizeof (WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, len );
    return wstr;
}

/************************************************************
 *                ADVAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
BOOL ADVAPI_IsLocalComputer(LPCWSTR ServerName)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Result;
    LPWSTR buf;

    if (!ServerName || !ServerName[0])
        return TRUE;

    buf = heap_alloc(dwSize * sizeof(WCHAR));
    Result = GetComputerNameW(buf,  &dwSize);
    if (Result && (ServerName[0] == '\\') && (ServerName[1] == '\\'))
        ServerName += 2;
    Result = Result && !lstrcmpW(ServerName, buf);
    heap_free(buf);

    return Result;
}

/************************************************************
 *                ADVAPI_GetComputerSid
 */
BOOL ADVAPI_GetComputerSid(PSID sid)
{
    static const struct /* same fields as struct SID */
    {
        BYTE Revision;
        BYTE SubAuthorityCount;
        SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
        DWORD SubAuthority[4];
    } computer_sid =
    { SID_REVISION, 4, { SECURITY_NT_AUTHORITY }, { SECURITY_NT_NON_UNIQUE, 0, 0, 0 } };

    memcpy( sid, &computer_sid, sizeof(computer_sid) );
    return TRUE;
}

/* Exported functions */

/*
 * @implemented
 */
BOOL WINAPI
OpenProcessToken(HANDLE ProcessHandle,
                 DWORD DesiredAccess,
                 PHANDLE TokenHandle)
{
    NTSTATUS Status;

    TRACE("%p, %x, %p.\n", ProcessHandle, DesiredAccess, TokenHandle);

    Status = NtOpenProcessToken(ProcessHandle,
                                DesiredAccess,
                                TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtOpenProcessToken failed! Status %08x.\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    TRACE("Returning token %p.\n", *TokenHandle);

    return TRUE;
}

/******************************************************************************
 * OpenThreadToken [ADVAPI32.@]
 *
 * Opens the access token associated with a thread handle.
 *
 * PARAMS
 *   ThreadHandle  [I] Handle to process
 *   DesiredAccess [I] Desired access to the thread
 *   OpenAsSelf    [I] ???
 *   TokenHandle   [O] Destination for the token handle
 *
 * RETURNS
 *  Success: TRUE. TokenHandle contains the access token.
 *  Failure: FALSE.
 *
 * NOTES
 *  See NtOpenThreadToken.
 */
BOOL WINAPI
OpenThreadToken( HANDLE ThreadHandle, DWORD DesiredAccess,
		 BOOL OpenAsSelf, HANDLE *TokenHandle)
{
	return set_ntstatus( NtOpenThreadToken(ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle));
}

/*
 * @implemented
 */
BOOL WINAPI
AdjustTokenGroups(HANDLE TokenHandle,
                  BOOL ResetToDefault,
                  PTOKEN_GROUPS NewState,
                  DWORD BufferLength,
                  PTOKEN_GROUPS PreviousState,
                  PDWORD ReturnLength)
{
    NTSTATUS Status;

    Status = NtAdjustGroupsToken(TokenHandle,
                                 ResetToDefault,
                                 NewState,
                                 BufferLength,
                                 PreviousState,
                                 (PULONG)ReturnLength);
    if (!NT_SUCCESS(Status))
    {
       SetLastError(RtlNtStatusToDosError(Status));
       return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AdjustTokenPrivileges(HANDLE TokenHandle,
                      BOOL DisableAllPrivileges,
                      PTOKEN_PRIVILEGES NewState,
                      DWORD BufferLength,
                      PTOKEN_PRIVILEGES PreviousState,
                      PDWORD ReturnLength)
{
    NTSTATUS Status;

    Status = NtAdjustPrivilegesToken(TokenHandle,
                                     DisableAllPrivileges,
                                     NewState,
                                     BufferLength,
                                     PreviousState,
                                     (PULONG)ReturnLength);
    if (STATUS_NOT_ALL_ASSIGNED == Status)
    {
        SetLastError(ERROR_NOT_ALL_ASSIGNED);
        return TRUE;
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* AdjustTokenPrivileges is documented to do this */
    SetLastError(ERROR_SUCCESS);

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
GetTokenInformation(HANDLE TokenHandle,
                    TOKEN_INFORMATION_CLASS TokenInformationClass,
                    LPVOID TokenInformation,
                    DWORD TokenInformationLength,
                    PDWORD ReturnLength)
{
    NTSTATUS Status;

    Status = NtQueryInformationToken(TokenHandle,
                                     TokenInformationClass,
                                     TokenInformation,
                                     TokenInformationLength,
                                     (PULONG)ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

  return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
SetTokenInformation(HANDLE TokenHandle,
                    TOKEN_INFORMATION_CLASS TokenInformationClass,
                    LPVOID TokenInformation,
                    DWORD TokenInformationLength)
{
    NTSTATUS Status;

    Status = NtSetInformationToken(TokenHandle,
                                   TokenInformationClass,
                                   TokenInformation,
                                   TokenInformationLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
SetThreadToken(IN PHANDLE ThreadHandle  OPTIONAL,
               IN HANDLE TokenHandle)
{
    NTSTATUS Status;
    HANDLE hThread;

    hThread = (ThreadHandle != NULL) ? *ThreadHandle : NtCurrentThread();

    Status = NtSetInformationThread(hThread,
                                    ThreadImpersonationToken,
                                    &TokenHandle,
                                    sizeof(HANDLE));
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief
 * Creates a filtered token that is a restricted one
 * of the regular access token. A restricted token
 * can have disabled SIDs, deleted privileges and/or
 * restricted SIDs added.
 *
 * @param[in] ExistingTokenHandle
 * An existing handle to a token where it's to be
 * filtered.
 *
 * @param[in] Flags
 * Privilege flag options. This parameter argument influences how the token
 * is filtered. Such parameter can be 0.
 *
 * @param[in] DisableSidCount
 * The count number of SIDs to disable.
 *
 * @param[in] SidsToDisable
 * An array list with SIDs that have to be disabled in
 * a token.
 *
 * @param[in] DeletePrivilegeCount
 * The count number of privileges to be deleted.
 *
 * @param[in] PrivilegesToDelete
 * An array list with privileges that have to be deleted
 * in a token.
 *
 * @param[in] RestrictedSidCount
 * The count number of restricted SIDs.
 *
 * @param[in] SidsToRestrict
 * An array list with restricted SIDs to be added into
 * the token. If the token already has restricted SIDs
 * then the array provided by the caller is redundant
 * information alongside with the existing restricted
 * SIDs in the token.
 *
 * @param[out] NewTokenHandle
 * The newly received handle to a restricted (filtered)
 * token. The caller can use such handle to duplicate
 * a new token.
 *
 * @return
 * Returns TRUE if the function has successfully completed
 * the operations, otherwise FALSE is returned to indicate
 * failure. For further details the caller has to invoke
 * GetLastError() API call for extended information
 * about the failure.
 */
BOOL WINAPI CreateRestrictedToken(
    _In_ HANDLE ExistingTokenHandle,
    _In_ DWORD Flags,
    _In_ DWORD DisableSidCount,
    _In_reads_opt_(DisableSidCount) PSID_AND_ATTRIBUTES SidsToDisable,
    _In_ DWORD DeletePrivilegeCount,
    _In_reads_opt_(DeletePrivilegeCount) PLUID_AND_ATTRIBUTES PrivilegesToDelete,
    _In_ DWORD RestrictedSidCount,
    _In_reads_opt_(RestrictedSidCount) PSID_AND_ATTRIBUTES SidsToRestrict,
    _Outptr_ PHANDLE NewTokenHandle)
{
    NTSTATUS Status;
    BOOL Success;
    ULONG Index;
    PTOKEN_GROUPS DisableSids = NULL;
    PTOKEN_GROUPS RestrictedSids = NULL;
    PTOKEN_PRIVILEGES DeletePrivileges = NULL;

    /*
     * Capture the elements we're being given from
     * the caller and allocate the groups and/or
     * privileges that have to be filtered in
     * the token.
     */
    if (SidsToDisable != NULL)
    {
        DisableSids = (PTOKEN_GROUPS)LocalAlloc(LMEM_FIXED, DisableSidCount * sizeof(TOKEN_GROUPS));
        if (DisableSids == NULL)
        {
            /* We failed, bail out */
            SetLastError(RtlNtStatusToDosError(STATUS_INSUFFICIENT_RESOURCES));
            return FALSE;
        }

        /* Copy the counter and loop the elements to copy the rest */
        DisableSids->GroupCount = DisableSidCount;
        for (Index = 0; Index < DisableSidCount; Index++)
        {
            DisableSids->Groups[Index].Sid = SidsToDisable[Index].Sid;
            DisableSids->Groups[Index].Attributes = SidsToDisable[Index].Attributes;
        }
    }

    if (PrivilegesToDelete != NULL)
    {
        DeletePrivileges = (PTOKEN_PRIVILEGES)LocalAlloc(LMEM_FIXED, DeletePrivilegeCount * sizeof(TOKEN_PRIVILEGES));
        if (DeletePrivileges == NULL)
        {
            /* We failed, bail out */
            SetLastError(RtlNtStatusToDosError(STATUS_INSUFFICIENT_RESOURCES));
            Success = FALSE;
            goto Cleanup;
        }

        /* Copy the counter and loop the elements to copy the rest */
        DeletePrivileges->PrivilegeCount = DeletePrivilegeCount;
        for (Index = 0; Index < DeletePrivilegeCount; Index++)
        {
            DeletePrivileges->Privileges[Index].Luid = PrivilegesToDelete[Index].Luid;
            DeletePrivileges->Privileges[Index].Attributes = PrivilegesToDelete[Index].Attributes;
        }
    }

    if (SidsToRestrict != NULL)
    {
        RestrictedSids = (PTOKEN_GROUPS)LocalAlloc(LMEM_FIXED, RestrictedSidCount * sizeof(TOKEN_GROUPS));
        if (RestrictedSids == NULL)
        {
            /* We failed, bail out */
            SetLastError(RtlNtStatusToDosError(STATUS_INSUFFICIENT_RESOURCES));
            Success = FALSE;
            goto Cleanup;
        }

        /* Copy the counter and loop the elements to copy the rest */
        RestrictedSids->GroupCount = RestrictedSidCount;
        for (Index = 0; Index < RestrictedSidCount; Index++)
        {
            RestrictedSids->Groups[Index].Sid = SidsToRestrict[Index].Sid;
            RestrictedSids->Groups[Index].Attributes = SidsToRestrict[Index].Attributes;
        }
    }

    /*
     * Call the NT API to request a token filtering
     * operation for us.
     */
    Status = NtFilterToken(ExistingTokenHandle,
                           Flags,
                           DisableSids,
                           DeletePrivileges,
                           RestrictedSids,
                           NewTokenHandle);
    if (!NT_SUCCESS(Status))
    {
        /* We failed to do the job, bail out */
        SetLastError(RtlNtStatusToDosError(Status));
        Success = FALSE;
        goto Cleanup;
    }

    /* If we reach here then we've successfully filtered the token */
    Success = TRUE;

Cleanup:
    /* Free whatever we allocated before */
    if (DisableSids != NULL)
    {
        LocalFree(DisableSids);
    }

    if (DeletePrivileges != NULL)
    {
        LocalFree(DeletePrivileges);
    }

    if (RestrictedSids != NULL)
    {
        LocalFree(RestrictedSids);
    }

    return Success;
}

/******************************************************************************
 * AllocateAndInitializeSid [ADVAPI32.@]
 *
 * PARAMS
 *   pIdentifierAuthority []
 *   nSubAuthorityCount   []
 *   nSubAuthority0       []
 *   nSubAuthority1       []
 *   nSubAuthority2       []
 *   nSubAuthority3       []
 *   nSubAuthority4       []
 *   nSubAuthority5       []
 *   nSubAuthority6       []
 *   nSubAuthority7       []
 *   pSid                 []
 */
BOOL WINAPI
AllocateAndInitializeSid( PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
                          BYTE nSubAuthorityCount,
                          DWORD nSubAuthority0, DWORD nSubAuthority1,
                          DWORD nSubAuthority2, DWORD nSubAuthority3,
                          DWORD nSubAuthority4, DWORD nSubAuthority5,
                          DWORD nSubAuthority6, DWORD nSubAuthority7,
                          PSID *pSid )
{
    return set_ntstatus( RtlAllocateAndInitializeSid(
                             pIdentifierAuthority, nSubAuthorityCount,
                             nSubAuthority0, nSubAuthority1, nSubAuthority2, nSubAuthority3,
                             nSubAuthority4, nSubAuthority5, nSubAuthority6, nSubAuthority7,
                             pSid ));
}

/*
 * @implemented
 *
 * RETURNS
 *  Docs says this function does NOT return a value
 *  even thou it's defined to return a PVOID...
 */
PVOID
WINAPI
FreeSid(PSID pSid)
{
    return RtlFreeSid(pSid);
}

/******************************************************************************
 * CopySid [ADVAPI32.@]
 *
 * PARAMS
 *   nDestinationSidLength []
 *   pDestinationSid       []
 *   pSourceSid            []
 */
BOOL WINAPI
CopySid( DWORD nDestinationSidLength, PSID pDestinationSid, PSID pSourceSid )
{
	return set_ntstatus(RtlCopySid(nDestinationSidLength, pDestinationSid, pSourceSid));
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CreateWellKnownSid(IN WELL_KNOWN_SID_TYPE WellKnownSidType,
                   IN PSID DomainSid  OPTIONAL,
                   OUT PSID pSid,
                   IN OUT DWORD* cbSid)
{
    unsigned int i;
    TRACE("(%d, %s, %p, %p)\n", WellKnownSidType, debugstr_sid(DomainSid), pSid, cbSid);

    if (cbSid == NULL || (DomainSid && !IsValidSid(DomainSid)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < sizeof(WellKnownSids)/sizeof(WellKnownSids[0]); i++) {
        if (WellKnownSids[i].Type == WellKnownSidType) {
            DWORD length = GetSidLengthRequired(WellKnownSids[i].Sid.SubAuthorityCount);

            if (*cbSid < length)
            {
                *cbSid = length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!pSid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(pSid, &WellKnownSids[i].Sid.Revision, length);
            *cbSid = length;
            return TRUE;
        }
    }

    if (DomainSid == NULL || *GetSidSubAuthorityCount(DomainSid) == SID_MAX_SUB_AUTHORITIES)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < sizeof(WellKnownRids)/sizeof(WellKnownRids[0]); i++)
        if (WellKnownRids[i].Type == WellKnownSidType) {
            UCHAR domain_subauth = *GetSidSubAuthorityCount(DomainSid);
            DWORD domain_sid_length = GetSidLengthRequired(domain_subauth);
            DWORD output_sid_length = GetSidLengthRequired(domain_subauth + 1);

            if (*cbSid < output_sid_length)
            {
                *cbSid = output_sid_length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!pSid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(pSid, DomainSid, domain_sid_length);
            (*GetSidSubAuthorityCount(pSid))++;
            (*GetSidSubAuthority(pSid, domain_subauth)) = WellKnownRids[i].Rid;
            *cbSid = output_sid_length;
            return TRUE;
        }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IsWellKnownSid(IN PSID pSid,
               IN WELL_KNOWN_SID_TYPE WellKnownSidType)
{
    unsigned int i;
    TRACE("(%s, %d)\n", debugstr_sid(pSid), WellKnownSidType);

    for (i = 0; i < sizeof(WellKnownSids) / sizeof(WellKnownSids[0]); i++)
    {
        if (WellKnownSids[i].Type == WellKnownSidType)
        {
            if (EqualSid(pSid, (PSID)(&WellKnownSids[i].Sid.Revision)))
                return TRUE;
        }
    }

    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsValidSid(PSID pSid)
{
    return (BOOL)RtlValidSid(pSid);
}

/*
 * @implemented
 */
BOOL
WINAPI
EqualSid(PSID pSid1,
         PSID pSid2)
{
    SetLastError(ERROR_SUCCESS);
    return RtlEqualSid (pSid1, pSid2);
}

/*
 * @implemented
 */
BOOL
WINAPI
EqualPrefixSid(PSID pSid1,
               PSID pSid2)
{
    return RtlEqualPrefixSid (pSid1, pSid2);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetSidLengthRequired(UCHAR nSubAuthorityCount)
{
    return (DWORD)RtlLengthRequiredSid(nSubAuthorityCount);
}

/*
 * @implemented
 */
BOOL
WINAPI
InitializeSid(PSID Sid,
              PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
              BYTE nSubAuthorityCount)
{
    NTSTATUS Status;

    Status = RtlInitializeSid(Sid,
                              pIdentifierAuthority,
                              nSubAuthorityCount);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
PSID_IDENTIFIER_AUTHORITY
WINAPI
GetSidIdentifierAuthority(PSID pSid)
{
    SetLastError(ERROR_SUCCESS);
    return RtlIdentifierAuthoritySid(pSid);
}

/*
 * @implemented
 */
PDWORD
WINAPI
GetSidSubAuthority(PSID pSid,
                   DWORD nSubAuthority)
{
    SetLastError(ERROR_SUCCESS);
    return (PDWORD)RtlSubAuthoritySid(pSid, nSubAuthority);
}

/*
 * @implemented
 */
PUCHAR
WINAPI
GetSidSubAuthorityCount(PSID pSid)
{
    SetLastError(ERROR_SUCCESS);
    return RtlSubAuthorityCountSid(pSid);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetLengthSid(PSID pSid)
{
    return (DWORD)RtlLengthSid(pSid);
}

/*
 * @implemented
 */
BOOL
WINAPI
InitializeSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                             DWORD dwRevision)
{
    NTSTATUS Status;

    Status = RtlCreateSecurityDescriptor(pSecurityDescriptor,
                                         dwRevision);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
MakeAbsoluteSD(PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
               PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
               LPDWORD lpdwAbsoluteSecurityDescriptorSize,
               PACL pDacl,
               LPDWORD lpdwDaclSize,
               PACL pSacl,
               LPDWORD lpdwSaclSize,
               PSID pOwner,
               LPDWORD lpdwOwnerSize,
               PSID pPrimaryGroup,
               LPDWORD lpdwPrimaryGroupSize)
{
    NTSTATUS Status;

    Status = RtlSelfRelativeToAbsoluteSD(pSelfRelativeSecurityDescriptor,
                                         pAbsoluteSecurityDescriptor,
                                         lpdwAbsoluteSecurityDescriptorSize,
                                         pDacl,
                                         lpdwDaclSize,
                                         pSacl,
                                         lpdwSaclSize,
                                         pOwner,
                                         lpdwOwnerSize,
                                         pPrimaryGroup,
                                         lpdwPrimaryGroupSize);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * GetKernelObjectSecurity [ADVAPI32.@]
 */
BOOL WINAPI GetKernelObjectSecurity(
        HANDLE Handle,
        SECURITY_INFORMATION RequestedInformation,
        PSECURITY_DESCRIPTOR pSecurityDescriptor,
        DWORD nLength,
        LPDWORD lpnLengthNeeded )
{
    TRACE("(%p,0x%08x,%p,0x%08x,%p)\n", Handle, RequestedInformation,
          pSecurityDescriptor, nLength, lpnLengthNeeded);

    return set_ntstatus( NtQuerySecurityObject(Handle, RequestedInformation, pSecurityDescriptor,
                                               nLength, lpnLengthNeeded ));
}

/*
 * @implemented
 */
BOOL
WINAPI
InitializeAcl(PACL pAcl,
              DWORD nAclLength,
              DWORD dwAclRevision)
{
    NTSTATUS Status;

    Status = RtlCreateAcl(pAcl,
                          nAclLength,
                          dwAclRevision);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI ImpersonateNamedPipeClient( HANDLE hNamedPipe )
{
    IO_STATUS_BLOCK io_block;

    TRACE("(%p)\n", hNamedPipe);

    return set_ntstatus( NtFsControlFile(hNamedPipe, NULL, NULL, NULL,
                         &io_block, FSCTL_PIPE_IMPERSONATE, NULL, 0, NULL, 0) );
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAccessAllowedAce(PACL pAcl,
                    DWORD dwAceRevision,
                    DWORD AccessMask,
                    PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessAllowedAce(pAcl,
                                    dwAceRevision,
                                    AccessMask,
                                    pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AddAccessAllowedAceEx(PACL pAcl,
                      DWORD dwAceRevision,
                      DWORD AceFlags,
                      DWORD AccessMask,
                      PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessAllowedAceEx(pAcl,
                                      dwAceRevision,
                                      AceFlags,
                                      AccessMask,
                                      pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAccessDeniedAce(PACL pAcl,
                   DWORD dwAceRevision,
                   DWORD AccessMask,
                   PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessDeniedAce(pAcl,
                                   dwAceRevision,
                                   AccessMask,
                                   pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AddAccessDeniedAceEx(PACL pAcl,
                     DWORD dwAceRevision,
                     DWORD AceFlags,
                     DWORD AccessMask,
                     PSID pSid)
{
    NTSTATUS Status;

    Status = RtlAddAccessDeniedAceEx(pAcl,
                                     dwAceRevision,
                                     AceFlags,
                                     AccessMask,
                                     pSid);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAce(PACL pAcl,
       DWORD dwAceRevision,
       DWORD dwStartingAceIndex,
       LPVOID pAceList,
       DWORD nAceListLength)
{
    NTSTATUS Status;

    Status = RtlAddAce(pAcl,
                       dwAceRevision,
                       dwStartingAceIndex,
                       pAceList,
                       nAceListLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * DeleteAce [ADVAPI32.@]
 */
BOOL WINAPI DeleteAce(PACL pAcl, DWORD dwAceIndex)
{
    return set_ntstatus(RtlDeleteAce(pAcl, dwAceIndex));
}

/*
 * @implemented
 */
BOOL
WINAPI
FindFirstFreeAce(PACL pAcl,
                 LPVOID *pAce)
{
    return RtlFirstFreeAce(pAcl,
                           (PACE*)pAce);
}

/******************************************************************************
 * GetAce [ADVAPI32.@]
 */
BOOL WINAPI GetAce(PACL pAcl,DWORD dwAceIndex,LPVOID *pAce )
{
    return set_ntstatus(RtlGetAce(pAcl, dwAceIndex, pAce));
}

/******************************************************************************
 * GetAclInformation [ADVAPI32.@]
 */
BOOL WINAPI GetAclInformation(
  PACL pAcl,
  LPVOID pAclInformation,
  DWORD nAclInformationLength,
  ACL_INFORMATION_CLASS dwAclInformationClass)
{
    return set_ntstatus(RtlQueryInformationAcl(pAcl, pAclInformation,
                                               nAclInformationLength, dwAclInformationClass));
}

/*
 * @implemented
 */
BOOL
WINAPI
IsValidAcl(PACL pAcl)
{
    return RtlValidAcl (pAcl);
}

/*
 * @implemented
 */
BOOL WINAPI
AllocateLocallyUniqueId(PLUID Luid)
{
    NTSTATUS Status;

    Status = NtAllocateLocallyUniqueId (Luid);
    if (!NT_SUCCESS (Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 * LookupPrivilegeDisplayNameA			EXPORTED
 *
 * @unimplemented
 */
BOOL
WINAPI
LookupPrivilegeDisplayNameA(LPCSTR lpSystemName,
                            LPCSTR lpName,
                            LPSTR lpDisplayName,
                            LPDWORD cchDisplayName,
                            LPDWORD lpLanguageId)
{
    UNICODE_STRING lpSystemNameW;
    UNICODE_STRING lpNameW;
    BOOL ret;
    DWORD wLen = 0;

    TRACE("%s %s %p %p %p\n", debugstr_a(lpSystemName), debugstr_a(lpName), lpName, cchDisplayName, lpLanguageId);

    RtlCreateUnicodeStringFromAsciiz(&lpSystemNameW, lpSystemName);
    RtlCreateUnicodeStringFromAsciiz(&lpNameW, lpName);
    ret = LookupPrivilegeDisplayNameW(lpSystemNameW.Buffer, lpNameW.Buffer, NULL, &wLen, lpLanguageId);
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        LPWSTR lpDisplayNameW = HeapAlloc(GetProcessHeap(), 0, wLen * sizeof(WCHAR));

        ret = LookupPrivilegeDisplayNameW(lpSystemNameW.Buffer, lpNameW.Buffer, lpDisplayNameW,
                                          &wLen, lpLanguageId);
        if (ret)
        {
            unsigned int len = WideCharToMultiByte(CP_ACP, 0, lpDisplayNameW, -1, lpDisplayName,
                                                   *cchDisplayName, NULL, NULL);

            if (len == 0)
            {
                /* WideCharToMultiByte failed */
                ret = FALSE;
            }
            else if (len > *cchDisplayName)
            {
                *cchDisplayName = len;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                ret = FALSE;
            }
            else
            {
                /* WideCharToMultiByte succeeded, output length needs to be
                 * length not including NULL terminator
                 */
                *cchDisplayName = len - 1;
            }
        }
        HeapFree(GetProcessHeap(), 0, lpDisplayNameW);
    }
    RtlFreeUnicodeString(&lpSystemNameW);
    RtlFreeUnicodeString(&lpNameW);
    return ret;
}

/**********************************************************************
 * LookupPrivilegeNameA				EXPORTED
 *
 * @implemented
 */
BOOL
WINAPI
LookupPrivilegeNameA(LPCSTR lpSystemName,
                     PLUID lpLuid,
                     LPSTR lpName,
                     LPDWORD cchName)
{
    UNICODE_STRING lpSystemNameW;
    BOOL ret;
    DWORD wLen = 0;

    TRACE("%s %p %p %p\n", debugstr_a(lpSystemName), lpLuid, lpName, cchName);

    RtlCreateUnicodeStringFromAsciiz(&lpSystemNameW, lpSystemName);
    ret = LookupPrivilegeNameW(lpSystemNameW.Buffer, lpLuid, NULL, &wLen);
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        LPWSTR lpNameW = HeapAlloc(GetProcessHeap(), 0, wLen * sizeof(WCHAR));

        ret = LookupPrivilegeNameW(lpSystemNameW.Buffer, lpLuid, lpNameW,
         &wLen);
        if (ret)
        {
            /* Windows crashes if cchName is NULL, so will I */
            unsigned int len = WideCharToMultiByte(CP_ACP, 0, lpNameW, -1, lpName,
             *cchName, NULL, NULL);

            if (len == 0)
            {
                /* WideCharToMultiByte failed */
                ret = FALSE;
            }
            else if (len > *cchName)
            {
                *cchName = len;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                ret = FALSE;
            }
            else
            {
                /* WideCharToMultiByte succeeded, output length needs to be
                 * length not including NULL terminator
                 */
                *cchName = len - 1;
            }
        }
        HeapFree(GetProcessHeap(), 0, lpNameW);
    }
    RtlFreeUnicodeString(&lpSystemNameW);
    return ret;
}

/******************************************************************************
 * GetFileSecurityA [ADVAPI32.@]
 *
 * Obtains Specified information about the security of a file or directory.
 *
 * PARAMS
 *  lpFileName           [I] Name of the file to get info for
 *  RequestedInformation [I] SE_ flags from "winnt.h"
 *  pSecurityDescriptor  [O] Destination for security information
 *  nLength              [I] Length of pSecurityDescriptor
 *  lpnLengthNeeded      [O] Destination for length of returned security information
 *
 * RETURNS
 *  Success: TRUE. pSecurityDescriptor contains the requested information.
 *  Failure: FALSE. lpnLengthNeeded contains the required space to return the info.
 *
 * NOTES
 *  The information returned is constrained by the callers access rights and
 *  privileges.
 *
 * @implemented
 */
BOOL
WINAPI
GetFileSecurityA(LPCSTR lpFileName,
                 SECURITY_INFORMATION RequestedInformation,
                 PSECURITY_DESCRIPTOR pSecurityDescriptor,
                 DWORD nLength,
                 LPDWORD lpnLengthNeeded)
{
    UNICODE_STRING FileName;
    BOOL bResult;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    bResult = GetFileSecurityW(FileName.Buffer,
                               RequestedInformation,
                               pSecurityDescriptor,
                               nLength,
                               lpnLengthNeeded);

    RtlFreeUnicodeString(&FileName);

    return bResult;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetFileSecurityW(LPCWSTR lpFileName,
                 SECURITY_INFORMATION RequestedInformation,
                 PSECURITY_DESCRIPTOR pSecurityDescriptor,
                 DWORD nLength,
                 LPDWORD lpnLengthNeeded)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    UNICODE_STRING FileName;
    ULONG AccessMask = 0;
    HANDLE FileHandle;
    NTSTATUS Status;

    TRACE("GetFileSecurityW() called\n");

    QuerySecurityAccessMask(RequestedInformation, &AccessMask);

    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &FileName,
                                      NULL,
                                      NULL))
    {
        ERR("Invalid path\n");
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        AccessMask,
                        &ObjectAttributes,
                        &StatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        0);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                FileName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        ERR("NtOpenFile() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    Status = NtQuerySecurityObject(FileHandle,
                                   RequestedInformation,
                                   pSecurityDescriptor,
                                   nLength,
                                   lpnLengthNeeded);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtQuerySecurityObject() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * SetFileSecurityA [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @implemented
 */
BOOL
WINAPI
SetFileSecurityA(LPCSTR lpFileName,
                 SECURITY_INFORMATION SecurityInformation,
                 PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    UNICODE_STRING FileName;
    BOOL bResult;

    if (!RtlCreateUnicodeStringFromAsciiz(&FileName, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    bResult = SetFileSecurityW(FileName.Buffer,
                               SecurityInformation,
                               pSecurityDescriptor);

    RtlFreeUnicodeString(&FileName);

    return bResult;
}

/******************************************************************************
 * SetFileSecurityW [ADVAPI32.@]
 * Sets the security of a file or directory
 *
 * @implemented
 */
BOOL
WINAPI
SetFileSecurityW(LPCWSTR lpFileName,
                 SECURITY_INFORMATION SecurityInformation,
                 PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    UNICODE_STRING FileName;
    ULONG AccessMask = 0;
    HANDLE FileHandle;
    NTSTATUS Status;

    TRACE("SetFileSecurityW() called\n");

    SetSecurityAccessMask(SecurityInformation, &AccessMask);

    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &FileName,
                                      NULL,
                                      NULL))
    {
        ERR("Invalid path\n");
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        AccessMask,
                        &ObjectAttributes,
                        &StatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        0);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                FileName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        ERR("NtOpenFile() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    Status = NtSetSecurityObject(FileHandle,
                                 SecurityInformation,
                                 pSecurityDescriptor);
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status))
    {
        ERR("NtSetSecurityObject() failed (Status %lx)\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * QueryWindows31FilesMigration [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 */
BOOL WINAPI
QueryWindows31FilesMigration( DWORD x1 )
{
	FIXME("(%d):stub\n",x1);
	return TRUE;
}

/******************************************************************************
 * SynchronizeWindows31FilesAndWindowsNTRegistry [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 */
BOOL WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry( DWORD x1, DWORD x2, DWORD x3,
                                               DWORD x4 )
{
	FIXME("(0x%08x,0x%08x,0x%08x,0x%08x):stub\n",x1,x2,x3,x4);
	return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RevertToSelf(VOID)
{
    NTSTATUS Status;
    HANDLE Token = NULL;

    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    &Token,
                                    sizeof(HANDLE));
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ImpersonateSelf(SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    NTSTATUS Status;

    Status = RtlImpersonateSelf(ImpersonationLevel);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AccessCheck(IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
            IN HANDLE ClientToken,
            IN DWORD DesiredAccess,
            IN PGENERIC_MAPPING GenericMapping,
            OUT PPRIVILEGE_SET PrivilegeSet OPTIONAL,
            IN OUT LPDWORD PrivilegeSetLength,
            OUT LPDWORD GrantedAccess,
            OUT LPBOOL AccessStatus)
{
    NTSTATUS Status;
    NTSTATUS NtAccessStatus;

    /* Do the access check */
    Status = NtAccessCheck(pSecurityDescriptor,
                           ClientToken,
                           DesiredAccess,
                           GenericMapping,
                           PrivilegeSet,
                           (PULONG)PrivilegeSetLength,
                           (PACCESS_MASK)GrantedAccess,
                           &NtAccessStatus);

    /* See if the access check operation succeeded */
    if (!NT_SUCCESS(Status))
    {
        /* Check failed */
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Now check the access status  */
    if (!NT_SUCCESS(NtAccessStatus))
    {
        /* Access denied */
        SetLastError(RtlNtStatusToDosError(NtAccessStatus));
        *AccessStatus = FALSE;
    }
    else
    {
        /* Access granted */
        *AccessStatus = TRUE;
    }

    /* Check succeeded */
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL WINAPI AccessCheckByType(
    PSECURITY_DESCRIPTOR pSecurityDescriptor, 
    PSID PrincipalSelfSid,
    HANDLE ClientToken, 
    DWORD DesiredAccess, 
    POBJECT_TYPE_LIST ObjectTypeList,
    DWORD ObjectTypeListLength,
    PGENERIC_MAPPING GenericMapping,
    PPRIVILEGE_SET PrivilegeSet,
    LPDWORD PrivilegeSetLength, 
    LPDWORD GrantedAccess,
    LPBOOL AccessStatus)
{
	FIXME("stub\n");

	*AccessStatus = TRUE;

	return !*AccessStatus;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetKernelObjectSecurity(HANDLE Handle,
                        SECURITY_INFORMATION SecurityInformation,
                        PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    NTSTATUS Status;

    Status = NtSetSecurityObject(Handle,
                                 SecurityInformation,
                                 SecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AddAuditAccessAce(PACL pAcl,
                  DWORD dwAceRevision,
                  DWORD dwAccessMask,
                  PSID pSid,
                  BOOL bAuditSuccess,
                  BOOL bAuditFailure)
{
    NTSTATUS Status;

    Status = RtlAddAuditAccessAce(pAcl,
                                  dwAceRevision,
                                  dwAccessMask,
                                  pSid,
                                  bAuditSuccess,
                                  bAuditFailure);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
AddAuditAccessAceEx(PACL pAcl,
                    DWORD dwAceRevision,
                    DWORD AceFlags,
                    DWORD dwAccessMask,
                    PSID pSid,
                    BOOL bAuditSuccess,
                    BOOL bAuditFailure)
{
    NTSTATUS Status;

    Status = RtlAddAuditAccessAceEx(pAcl,
                                    dwAceRevision,
                                    AceFlags,
                                    dwAccessMask,
                                    pSid,
                                    bAuditSuccess,
                                    bAuditFailure);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * LookupAccountNameA [ADVAPI32.@]
 *
 * @implemented
 */
BOOL
WINAPI
LookupAccountNameA(LPCSTR SystemName,
                   LPCSTR AccountName,
                   PSID Sid,
                   LPDWORD SidLength,
                   LPSTR ReferencedDomainName,
                   LPDWORD hReferencedDomainNameLength,
                   PSID_NAME_USE SidNameUse)
{
    BOOL ret;
    UNICODE_STRING lpSystemW;
    UNICODE_STRING lpAccountW;
    LPWSTR lpReferencedDomainNameW = NULL;

    RtlCreateUnicodeStringFromAsciiz(&lpSystemW, SystemName);
    RtlCreateUnicodeStringFromAsciiz(&lpAccountW, AccountName);

    if (ReferencedDomainName)
        lpReferencedDomainNameW = HeapAlloc(GetProcessHeap(),
                                            0,
                                            *hReferencedDomainNameLength * sizeof(WCHAR));

    ret = LookupAccountNameW(lpSystemW.Buffer,
                             lpAccountW.Buffer,
                             Sid,
                             SidLength,
                             lpReferencedDomainNameW,
                             hReferencedDomainNameLength,
                             SidNameUse);

    if (ret && lpReferencedDomainNameW)
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            lpReferencedDomainNameW,
                            *hReferencedDomainNameLength + 1,
                            ReferencedDomainName,
                            *hReferencedDomainNameLength + 1,
                            NULL,
                            NULL);
    }

    RtlFreeUnicodeString(&lpSystemW);
    RtlFreeUnicodeString(&lpAccountW);
    HeapFree(GetProcessHeap(), 0, lpReferencedDomainNameW);

    return ret;
}

/**********************************************************************
 *	PrivilegeCheck					EXPORTED
 *
 * @implemented
 */
BOOL WINAPI
PrivilegeCheck(HANDLE ClientToken,
               PPRIVILEGE_SET RequiredPrivileges,
               LPBOOL pfResult)
{
    BOOLEAN Result;
    NTSTATUS Status;

    Status = NtPrivilegeCheck(ClientToken,
                              RequiredPrivileges,
                              &Result);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *pfResult = (BOOL)Result;

    return TRUE;
}

/******************************************************************************
 * GetSecurityInfoExW         EXPORTED
 */
DWORD
WINAPI
GetSecurityInfoExA(HANDLE hObject,
                   SE_OBJECT_TYPE ObjectType,
                   SECURITY_INFORMATION SecurityInfo,
                   LPCSTR lpProvider,
                   LPCSTR lpProperty,
                   PACTRL_ACCESSA *ppAccessList,
                   PACTRL_AUDITA *ppAuditList,
                   LPSTR *lppOwner,
                   LPSTR *lppGroup)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_BAD_PROVIDER;
}


/******************************************************************************
 * GetSecurityInfoExW         EXPORTED
 */
DWORD
WINAPI
GetSecurityInfoExW(HANDLE hObject,
                   SE_OBJECT_TYPE ObjectType,
                   SECURITY_INFORMATION SecurityInfo,
                   LPCWSTR lpProvider,
                   LPCWSTR lpProperty,
                   PACTRL_ACCESSW *ppAccessList,
                   PACTRL_AUDITW *ppAuditList,
                   LPWSTR *lppOwner,
                   LPWSTR *lppGroup)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return ERROR_BAD_PROVIDER;
}

/******************************************************************************
 * BuildExplicitAccessWithNameA [ADVAPI32.@]
 */
VOID WINAPI
BuildExplicitAccessWithNameA(PEXPLICIT_ACCESSA pExplicitAccess,
                             LPSTR pTrusteeName,
                             DWORD AccessPermissions,
                             ACCESS_MODE AccessMode,
                             DWORD Inheritance)
{
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;

    pExplicitAccess->Trustee.pMultipleTrustee = NULL;
    pExplicitAccess->Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pExplicitAccess->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    pExplicitAccess->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    pExplicitAccess->Trustee.ptstrName = pTrusteeName;
}


/******************************************************************************
 * BuildExplicitAccessWithNameW [ADVAPI32.@]
 */
VOID WINAPI
BuildExplicitAccessWithNameW(PEXPLICIT_ACCESSW pExplicitAccess,
                             LPWSTR pTrusteeName,
                             DWORD AccessPermissions,
                             ACCESS_MODE AccessMode,
                             DWORD Inheritance)
{
    pExplicitAccess->grfAccessPermissions = AccessPermissions;
    pExplicitAccess->grfAccessMode = AccessMode;
    pExplicitAccess->grfInheritance = Inheritance;

    pExplicitAccess->Trustee.pMultipleTrustee = NULL;
    pExplicitAccess->Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pExplicitAccess->Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    pExplicitAccess->Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    pExplicitAccess->Trustee.ptstrName = pTrusteeName;
}

/******************************************************************************
 * BuildTrusteeWithObjectsAndNameA [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithObjectsAndNameA( PTRUSTEEA pTrustee, POBJECTS_AND_NAME_A pObjName,
                                             SE_OBJECT_TYPE ObjectType, LPSTR ObjectTypeName,
                                             LPSTR InheritedObjectTypeName, LPSTR Name )
{
    DWORD ObjectsPresent = 0;

    TRACE("%p %p 0x%08x %p %p %s\n", pTrustee, pObjName,
          ObjectType, ObjectTypeName, InheritedObjectTypeName, debugstr_a(Name));

    /* Fill the OBJECTS_AND_NAME structure */
    pObjName->ObjectType = ObjectType;
    if (ObjectTypeName != NULL)
    {
        ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }

    pObjName->InheritedObjectTypeName = InheritedObjectTypeName;
    if (InheritedObjectTypeName != NULL)
    {
        ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }

    pObjName->ObjectsPresent = ObjectsPresent;
    pObjName->ptstrName = Name;

    /* Fill the TRUSTEE structure */
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR)pObjName;
}

/******************************************************************************
 * BuildTrusteeWithObjectsAndNameW [ADVAPI32.@]
 */
VOID WINAPI BuildTrusteeWithObjectsAndNameW( PTRUSTEEW pTrustee, POBJECTS_AND_NAME_W pObjName,
                                             SE_OBJECT_TYPE ObjectType, LPWSTR ObjectTypeName,
                                             LPWSTR InheritedObjectTypeName, LPWSTR Name )
{
    DWORD ObjectsPresent = 0;

    TRACE("%p %p 0x%08x %p %p %s\n", pTrustee, pObjName,
          ObjectType, ObjectTypeName, InheritedObjectTypeName, debugstr_w(Name));

    /* Fill the OBJECTS_AND_NAME structure */
    pObjName->ObjectType = ObjectType;
    if (ObjectTypeName != NULL)
    {
        ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }

    pObjName->InheritedObjectTypeName = InheritedObjectTypeName;
    if (InheritedObjectTypeName != NULL)
    {
        ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }

    pObjName->ObjectsPresent = ObjectsPresent;
    pObjName->ptstrName = Name;

    /* Fill the TRUSTEE structure */
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR)pObjName;
}

/******************************************************************************
 * BuildTrusteeWithObjectsAndSidA [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithObjectsAndSidA(PTRUSTEEA pTrustee,
                               POBJECTS_AND_SID pObjSid,
                               GUID *pObjectGuid,
                               GUID *pInheritedObjectGuid,
                               PSID pSid)
{
    DWORD ObjectsPresent = 0;

    TRACE("%p %p %p %p %p\n", pTrustee, pObjSid, pObjectGuid, pInheritedObjectGuid, pSid);

    /* Fill the OBJECTS_AND_SID structure */
    if (pObjectGuid != NULL)
    {
        pObjSid->ObjectTypeGuid = *pObjectGuid;
        ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }
    else
    {
        ZeroMemory(&pObjSid->ObjectTypeGuid,
                   sizeof(GUID));
    }

    if (pInheritedObjectGuid != NULL)
    {
        pObjSid->InheritedObjectTypeGuid = *pInheritedObjectGuid;
        ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }
    else
    {
        ZeroMemory(&pObjSid->InheritedObjectTypeGuid,
                   sizeof(GUID));
    }

    pObjSid->ObjectsPresent = ObjectsPresent;
    pObjSid->pSid = pSid;

    /* Fill the TRUSTEE structure */
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR) pObjSid;
}


/******************************************************************************
 * BuildTrusteeWithObjectsAndSidW [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithObjectsAndSidW(PTRUSTEEW pTrustee,
                               POBJECTS_AND_SID pObjSid,
                               GUID *pObjectGuid,
                               GUID *pInheritedObjectGuid,
                               PSID pSid)
{
    DWORD ObjectsPresent = 0;

    TRACE("%p %p %p %p %p\n", pTrustee, pObjSid, pObjectGuid, pInheritedObjectGuid, pSid);

    /* Fill the OBJECTS_AND_SID structure */
    if (pObjectGuid != NULL)
    {
        pObjSid->ObjectTypeGuid = *pObjectGuid;
        ObjectsPresent |= ACE_OBJECT_TYPE_PRESENT;
    }
    else
    {
        ZeroMemory(&pObjSid->ObjectTypeGuid,
                   sizeof(GUID));
    }

    if (pInheritedObjectGuid != NULL)
    {
        pObjSid->InheritedObjectTypeGuid = *pInheritedObjectGuid;
        ObjectsPresent |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
    }
    else
    {
        ZeroMemory(&pObjSid->InheritedObjectTypeGuid,
                   sizeof(GUID));
    }

    pObjSid->ObjectsPresent = ObjectsPresent;
    pObjSid->pSid = pSid;

    /* Fill the TRUSTEE structure */
    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_OBJECTS_AND_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR) pObjSid;
}

/******************************************************************************
 * BuildTrusteeWithSidA [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithSidA(PTRUSTEE_A pTrustee,
                     PSID pSid)
{
    TRACE("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPSTR) pSid;
}


/******************************************************************************
 * BuildTrusteeWithSidW [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithSidW(PTRUSTEE_W pTrustee,
                     PSID pSid)
{
    TRACE("%p %p\n", pTrustee, pSid);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_SID;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = (LPWSTR) pSid;
}

/******************************************************************************
 * BuildTrusteeWithNameA [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithNameA(PTRUSTEE_A pTrustee,
                      LPSTR name)
{
    TRACE("%p %s\n", pTrustee, name);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}

/******************************************************************************
 * BuildTrusteeWithNameW [ADVAPI32.@]
 */
VOID WINAPI
BuildTrusteeWithNameW(PTRUSTEE_W pTrustee,
                      LPWSTR name)
{
    TRACE("%p %s\n", pTrustee, name);

    pTrustee->pMultipleTrustee = NULL;
    pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
    pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
    pTrustee->ptstrName = name;
}

/****************************************************************************** 
 * GetTrusteeFormA [ADVAPI32.@] 
 */ 
TRUSTEE_FORM WINAPI GetTrusteeFormA(PTRUSTEEA pTrustee) 
{  
    TRACE("(%p)\n", pTrustee); 
  
    if (!pTrustee) 
        return TRUSTEE_BAD_FORM; 
  
    return pTrustee->TrusteeForm; 
}  
  
/****************************************************************************** 
 * GetTrusteeFormW [ADVAPI32.@] 
 */ 
TRUSTEE_FORM WINAPI GetTrusteeFormW(PTRUSTEEW pTrustee) 
{  
    TRACE("(%p)\n", pTrustee); 
  
    if (!pTrustee) 
        return TRUSTEE_BAD_FORM; 
  
    return pTrustee->TrusteeForm; 
}  
  
/******************************************************************************
 * GetTrusteeNameA [ADVAPI32.@]
 */
LPSTR WINAPI
GetTrusteeNameA(PTRUSTEE_A pTrustee)
{
    return pTrustee->ptstrName;
}


/******************************************************************************
 * GetTrusteeNameW [ADVAPI32.@]
 */
LPWSTR WINAPI
GetTrusteeNameW(PTRUSTEE_W pTrustee)
{
    return pTrustee->ptstrName;
}

/******************************************************************************
 * GetTrusteeTypeA [ADVAPI32.@]
 */
TRUSTEE_TYPE WINAPI
GetTrusteeTypeA(PTRUSTEE_A pTrustee)
{
    return pTrustee->TrusteeType;
}

/******************************************************************************
 * GetTrusteeTypeW [ADVAPI32.@]
 */
TRUSTEE_TYPE WINAPI
GetTrusteeTypeW(PTRUSTEE_W pTrustee)
{
    return pTrustee->TrusteeType;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetAclInformation(PACL pAcl,
                  LPVOID pAclInformation,
                  DWORD nAclInformationLength,
                  ACL_INFORMATION_CLASS dwAclInformationClass)
{
    NTSTATUS Status;

    Status = RtlSetInformationAcl(pAcl,
                                  pAclInformation,
                                  nAclInformationLength,
                                  dwAclInformationClass);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 * SetNamedSecurityInfoA			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetNamedSecurityInfoA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID psidOwner,
                      PSID psidGroup,
                      PACL pDacl,
                      PACL pSacl)
{
    UNICODE_STRING ObjectName;
    DWORD Ret;

    if (!RtlCreateUnicodeStringFromAsciiz(&ObjectName, pObjectName))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Ret = SetNamedSecurityInfoW(ObjectName.Buffer,
                                ObjectType,
                                SecurityInfo,
                                psidOwner,
                                psidGroup,
                                pDacl,
                                pSacl);

    RtlFreeUnicodeString(&ObjectName);

    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
AreAllAccessesGranted(DWORD GrantedAccess,
                      DWORD DesiredAccess)
{
    return (BOOL)RtlAreAllAccessesGranted(GrantedAccess,
                                          DesiredAccess);
}

/*
 * @implemented
 */
BOOL
WINAPI
AreAnyAccessesGranted(DWORD GrantedAccess,
                      DWORD DesiredAccess)
{
    return (BOOL)RtlAreAnyAccessesGranted(GrantedAccess,
                                          DesiredAccess);
}

/******************************************************************************
 * ParseAclStringFlags
 */
static DWORD ParseAclStringFlags(LPCWSTR* StringAcl)
{
    DWORD flags = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl && *szAcl != '(')
    {
        if (*szAcl == 'P')
        {
            flags |= SE_DACL_PROTECTED;
        }
        else if (*szAcl == 'A')
        {
            szAcl++;
            if (*szAcl == 'R')
                flags |= SE_DACL_AUTO_INHERIT_REQ;
            else if (*szAcl == 'I')
                flags |= SE_DACL_AUTO_INHERITED;
        }
        szAcl++;
    }

    *StringAcl = szAcl;
    return flags;
}

/******************************************************************************
 * ParseAceStringType
 */
static const ACEFLAG AceType[] =
{
    { SDDL_ALARM,          SYSTEM_ALARM_ACE_TYPE },
    { SDDL_AUDIT,          SYSTEM_AUDIT_ACE_TYPE },
    { SDDL_ACCESS_ALLOWED, ACCESS_ALLOWED_ACE_TYPE },
    { SDDL_ACCESS_DENIED,  ACCESS_DENIED_ACE_TYPE },
    { SDDL_MANDATORY_LABEL,SYSTEM_MANDATORY_LABEL_ACE_TYPE },
    /*
    { SDDL_OBJECT_ACCESS_ALLOWED, ACCESS_ALLOWED_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_ACCESS_DENIED,  ACCESS_DENIED_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_ALARM,          SYSTEM_ALARM_OBJECT_ACE_TYPE },
    { SDDL_OBJECT_AUDIT,          SYSTEM_AUDIT_OBJECT_ACE_TYPE },
    */
    { NULL, 0 },
};

static BYTE ParseAceStringType(LPCWSTR* StringAcl)
{
    UINT len = 0;
    LPCWSTR szAcl = *StringAcl;
    const ACEFLAG *lpaf = AceType;

    while (*szAcl == ' ')
        szAcl++;

    while (lpaf->wstr &&
        (len = strlenW(lpaf->wstr)) &&
        strncmpW(lpaf->wstr, szAcl, len))
        lpaf++;

    if (!lpaf->wstr)
        return 0;

    *StringAcl = szAcl + len;
    return lpaf->value;
}


/******************************************************************************
 * ParseAceStringFlags
 */
static const ACEFLAG AceFlags[] =
{
    { SDDL_CONTAINER_INHERIT, CONTAINER_INHERIT_ACE },
    { SDDL_AUDIT_FAILURE,     FAILED_ACCESS_ACE_FLAG },
    { SDDL_INHERITED,         INHERITED_ACE },
    { SDDL_INHERIT_ONLY,      INHERIT_ONLY_ACE },
    { SDDL_NO_PROPAGATE,      NO_PROPAGATE_INHERIT_ACE },
    { SDDL_OBJECT_INHERIT,    OBJECT_INHERIT_ACE },
    { SDDL_AUDIT_SUCCESS,     SUCCESSFUL_ACCESS_ACE_FLAG },
    { NULL, 0 },
};

static BYTE ParseAceStringFlags(LPCWSTR* StringAcl)
{
    UINT len = 0;
    BYTE flags = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl == ' ')
        szAcl++;

    while (*szAcl != ';')
    {
        const ACEFLAG *lpaf = AceFlags;

        while (lpaf->wstr &&
               (len = strlenW(lpaf->wstr)) &&
               strncmpW(lpaf->wstr, szAcl, len))
            lpaf++;

        if (!lpaf->wstr)
            return 0;

	flags |= lpaf->value;
        szAcl += len;
    }

    *StringAcl = szAcl;
    return flags;
}


/******************************************************************************
 * ParseAceStringRights
 */
static const ACEFLAG AceRights[] =
{
    { SDDL_GENERIC_ALL,     GENERIC_ALL },
    { SDDL_GENERIC_READ,    GENERIC_READ },
    { SDDL_GENERIC_WRITE,   GENERIC_WRITE },
    { SDDL_GENERIC_EXECUTE, GENERIC_EXECUTE },

    { SDDL_READ_CONTROL,    READ_CONTROL },
    { SDDL_STANDARD_DELETE, DELETE },
    { SDDL_WRITE_DAC,       WRITE_DAC },
    { SDDL_WRITE_OWNER,     WRITE_OWNER },

    { SDDL_READ_PROPERTY,   ADS_RIGHT_DS_READ_PROP},
    { SDDL_WRITE_PROPERTY,  ADS_RIGHT_DS_WRITE_PROP},
    { SDDL_CREATE_CHILD,    ADS_RIGHT_DS_CREATE_CHILD},
    { SDDL_DELETE_CHILD,    ADS_RIGHT_DS_DELETE_CHILD},
    { SDDL_LIST_CHILDREN,   ADS_RIGHT_ACTRL_DS_LIST},
    { SDDL_SELF_WRITE,      ADS_RIGHT_DS_SELF},
    { SDDL_LIST_OBJECT,     ADS_RIGHT_DS_LIST_OBJECT},
    { SDDL_DELETE_TREE,     ADS_RIGHT_DS_DELETE_TREE},
    { SDDL_CONTROL_ACCESS,  ADS_RIGHT_DS_CONTROL_ACCESS},

    { SDDL_FILE_ALL,        FILE_ALL_ACCESS },
    { SDDL_FILE_READ,       FILE_GENERIC_READ },
    { SDDL_FILE_WRITE,      FILE_GENERIC_WRITE },
    { SDDL_FILE_EXECUTE,    FILE_GENERIC_EXECUTE },

    { SDDL_KEY_ALL,         KEY_ALL_ACCESS },
    { SDDL_KEY_READ,        KEY_READ },
    { SDDL_KEY_WRITE,       KEY_WRITE },
    { SDDL_KEY_EXECUTE,     KEY_EXECUTE },

    { SDDL_NO_READ_UP,      SYSTEM_MANDATORY_LABEL_NO_READ_UP },
    { SDDL_NO_WRITE_UP,     SYSTEM_MANDATORY_LABEL_NO_WRITE_UP },
    { SDDL_NO_EXECUTE_UP,   SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP },
    { NULL, 0 },
};

static DWORD ParseAceStringRights(LPCWSTR* StringAcl)
{
    UINT len = 0;
    DWORD rights = 0;
    LPCWSTR szAcl = *StringAcl;

    while (*szAcl == ' ')
        szAcl++;

    if ((*szAcl == '0') && (*(szAcl + 1) == 'x'))
    {
        LPCWSTR p = szAcl;

	while (*p && *p != ';')
            p++;

	if (p - szAcl <= 10 /* 8 hex digits + "0x" */ )
	{
	    rights = strtoulW(szAcl, NULL, 16);
	    szAcl = p;
	}
	else
            WARN("Invalid rights string format: %s\n", debugstr_wn(szAcl, p - szAcl));
    }
    else
    {
        while (*szAcl != ';')
        {
            const ACEFLAG *lpaf = AceRights;

            while (lpaf->wstr &&
               (len = strlenW(lpaf->wstr)) &&
               strncmpW(lpaf->wstr, szAcl, len))
	    {
               lpaf++;
	    }

            if (!lpaf->wstr)
                return 0;

	    rights |= lpaf->value;
            szAcl += len;
        }
    }

    *StringAcl = szAcl;
    return rights;
}


/******************************************************************************
 * ParseStringAclToAcl
 * 
 * dacl_flags(string_ace1)(string_ace2)... (string_acen) 
 */
static BOOL ParseStringAclToAcl(LPCWSTR StringAcl, LPDWORD lpdwFlags, 
    PACL pAcl, LPDWORD cBytes)
{
    DWORD val;
    DWORD sidlen;
    DWORD length = sizeof(ACL);
    DWORD acesize = 0;
    DWORD acecount = 0;
    PACCESS_ALLOWED_ACE pAce = NULL; /* pointer to current ACE */
    DWORD error = ERROR_INVALID_ACL;

    TRACE("%s\n", debugstr_w(StringAcl));

    if (!StringAcl)
	return FALSE;

    if (pAcl) /* pAce is only useful if we're setting values */
        pAce = (PACCESS_ALLOWED_ACE) (pAcl + 1);

    /* Parse ACL flags */
    *lpdwFlags = ParseAclStringFlags(&StringAcl);

    /* Parse ACE */
    while (*StringAcl == '(')
    {
        StringAcl++;

        /* Parse ACE type */
        val = ParseAceStringType(&StringAcl);
	if (pAce)
            pAce->Header.AceType = (BYTE) val;
        if (*StringAcl != ';')
        {
            error = RPC_S_INVALID_STRING_UUID;
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE flags */
	val = ParseAceStringFlags(&StringAcl);
	if (pAce)
            pAce->Header.AceFlags = (BYTE) val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE rights */
	val = ParseAceStringRights(&StringAcl);
	if (pAce)
            pAce->Mask = val;
        if (*StringAcl != ';')
            goto lerr;
        StringAcl++;

        /* Parse ACE object guid */
        while (*StringAcl == ' ')
            StringAcl++;
        if (*StringAcl != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE inherit object guid */
        while (*StringAcl == ' ')
            StringAcl++;
        if (*StringAcl != ';')
        {
            FIXME("Support for *_OBJECT_ACE_TYPE not implemented\n");
            goto lerr;
        }
        StringAcl++;

        /* Parse ACE account sid */
        if (ParseStringSidToSid(StringAcl, pAce ? &pAce->SidStart : NULL, &sidlen))
	{
            while (*StringAcl && *StringAcl != ')')
                StringAcl++;
	}

        if (*StringAcl != ')')
            goto lerr;
        StringAcl++;

        acesize = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + sidlen;
        length += acesize;
        if (pAce)
        {
            pAce->Header.AceSize = acesize;
            pAce = (PACCESS_ALLOWED_ACE)((LPBYTE)pAce + acesize);
        }
        acecount++;
    }

    *cBytes = length;

    if (length > 0xffff)
    {
        ERR("ACL too large\n");
        goto lerr;
    }

    if (pAcl)
    {
        pAcl->AclRevision = ACL_REVISION;
        pAcl->Sbz1 = 0;
        pAcl->AclSize = length;
        pAcl->AceCount = acecount;
        pAcl->Sbz2 = 0;
    }
    return TRUE;

lerr:
    SetLastError(error);
    WARN("Invalid ACE string format\n");
    return FALSE;
}

/******************************************************************************
 * ParseStringSecurityDescriptorToSecurityDescriptor
 */
static BOOL ParseStringSecurityDescriptorToSecurityDescriptor(
    LPCWSTR StringSecurityDescriptor,
    SECURITY_DESCRIPTOR_RELATIVE* SecurityDescriptor,
    LPDWORD cBytes)
{
    BOOL bret = FALSE;
    WCHAR toktype;
    WCHAR *tok;
    LPCWSTR lptoken;
    LPBYTE lpNext = NULL;
    DWORD len;

    *cBytes = sizeof(SECURITY_DESCRIPTOR_RELATIVE);

    tok = heap_alloc( (lstrlenW(StringSecurityDescriptor) + 1) * sizeof(WCHAR));

    if (SecurityDescriptor)
        lpNext = (LPBYTE)(SecurityDescriptor + 1);

    while (*StringSecurityDescriptor == ' ')
        StringSecurityDescriptor++;

    while (*StringSecurityDescriptor)
    {
        toktype = *StringSecurityDescriptor;

	/* Expect char identifier followed by ':' */
	StringSecurityDescriptor++;
        if (*StringSecurityDescriptor != ':')
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto lend;
        }
	StringSecurityDescriptor++;

	/* Extract token */
	lptoken = StringSecurityDescriptor;
	while (*lptoken && *lptoken != ':')
            lptoken++;

	if (*lptoken)
            lptoken--;

        len = lptoken - StringSecurityDescriptor;
        memcpy( tok, StringSecurityDescriptor, len * sizeof(WCHAR) );
        tok[len] = 0;

        switch (toktype)
	{
            case 'O':
            {
                DWORD bytes;

                if (!ParseStringSidToSid(tok, lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Owner = lpNext - (LPBYTE)SecurityDescriptor;
                    lpNext += bytes; /* Advance to next token */
                }

		*cBytes += bytes;

                break;
            }

            case 'G':
            {
                DWORD bytes;

                if (!ParseStringSidToSid(tok, lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Group = lpNext - (LPBYTE)SecurityDescriptor;
                    lpNext += bytes; /* Advance to next token */
                }

		*cBytes += bytes;

                break;
            }

            case 'D':
	    {
                DWORD flags;
                DWORD bytes;

                if (!ParseStringAclToAcl(tok, &flags, (PACL)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Control |= SE_DACL_PRESENT | flags;
                    SecurityDescriptor->Dacl = lpNext - (LPBYTE)SecurityDescriptor;
                    lpNext += bytes; /* Advance to next token */
		}

		*cBytes += bytes;

		break;
            }

            case 'S':
            {
                DWORD flags;
                DWORD bytes;

                if (!ParseStringAclToAcl(tok, &flags, (PACL)lpNext, &bytes))
                    goto lend;

                if (SecurityDescriptor)
                {
                    SecurityDescriptor->Control |= SE_SACL_PRESENT | flags;
                    SecurityDescriptor->Sacl = lpNext - (LPBYTE)SecurityDescriptor;
                    lpNext += bytes; /* Advance to next token */
		}

		*cBytes += bytes;

		break;
            }

            default:
                FIXME("Unknown token\n");
                SetLastError(ERROR_INVALID_PARAMETER);
		goto lend;
	}

        StringSecurityDescriptor = lptoken;
    }

    bret = TRUE;

lend:
    heap_free(tok);
    return bret;
}

/* Winehq cvs 20050916 */
/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorA [ADVAPI32.@]
 * @implemented
 */
BOOL
WINAPI
ConvertStringSecurityDescriptorToSecurityDescriptorA(LPCSTR StringSecurityDescriptor,
                                                     DWORD StringSDRevision,
                                                     PSECURITY_DESCRIPTOR* SecurityDescriptor,
                                                     PULONG SecurityDescriptorSize)
{
    UINT len;
    BOOL ret = FALSE;
    LPWSTR StringSecurityDescriptorW;

    len = MultiByteToWideChar(CP_ACP, 0, StringSecurityDescriptor, -1, NULL, 0);
    StringSecurityDescriptorW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

    if (StringSecurityDescriptorW)
    {
        MultiByteToWideChar(CP_ACP, 0, StringSecurityDescriptor, -1, StringSecurityDescriptorW, len);

        ret = ConvertStringSecurityDescriptorToSecurityDescriptorW(StringSecurityDescriptorW,
                                                                   StringSDRevision, SecurityDescriptor,
                                                                   SecurityDescriptorSize);
        HeapFree(GetProcessHeap(), 0, StringSecurityDescriptorW);
    }

    return ret;
}

/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorW [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI
ConvertStringSecurityDescriptorToSecurityDescriptorW(LPCWSTR StringSecurityDescriptor,
                                                     DWORD StringSDRevision,
                                                     PSECURITY_DESCRIPTOR* SecurityDescriptor,
                                                     PULONG SecurityDescriptorSize)
{
    DWORD cBytes;
    SECURITY_DESCRIPTOR* psd;
    BOOL bret = FALSE;

    TRACE("%s\n", debugstr_w(StringSecurityDescriptor));

    if (GetVersion() & 0x80000000)
    {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto lend;
    }
    else if (!StringSecurityDescriptor || !SecurityDescriptor)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto lend;
    }
    else if (StringSDRevision != SID_REVISION)
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
        goto lend;
    }

    /* Compute security descriptor length */
    if (!ParseStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
        NULL, &cBytes))
        goto lend;

    psd = *SecurityDescriptor = LocalAlloc(GMEM_ZEROINIT, cBytes);
    if (!psd) goto lend;

    psd->Revision = SID_REVISION;
    psd->Control |= SE_SELF_RELATIVE;

    if (!ParseStringSecurityDescriptorToSecurityDescriptor(StringSecurityDescriptor,
             (SECURITY_DESCRIPTOR_RELATIVE *)psd, &cBytes))
    {
        LocalFree(psd);
        goto lend;
    }

    if (SecurityDescriptorSize)
        *SecurityDescriptorSize = cBytes;

    bret = TRUE;

lend:
    TRACE(" ret=%d\n", bret);
    return bret;
}

static void DumpString(LPCWSTR string, int cch, WCHAR **pwptr, ULONG *plen)
{
    if (cch == -1)
        cch = strlenW(string);

    if (plen)
        *plen += cch;

    if (pwptr)
    {
        memcpy(*pwptr, string, sizeof(WCHAR)*cch);
        *pwptr += cch;
    }
}

static BOOL DumpSidNumeric(PSID psid, WCHAR **pwptr, ULONG *plen)
{
    DWORD i;
    WCHAR fmt[] = { 'S','-','%','u','-','%','d',0 };
    WCHAR subauthfmt[] = { '-','%','u',0 };
    WCHAR buf[26];
    SID *pisid = psid;

    if( !IsValidSid( psid ) || pisid->Revision != SDDL_REVISION)
    {
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    if (pisid->IdentifierAuthority.Value[0] ||
     pisid->IdentifierAuthority.Value[1])
    {
        FIXME("not matching MS' bugs\n");
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    sprintfW( buf, fmt, pisid->Revision,
        MAKELONG(
            MAKEWORD( pisid->IdentifierAuthority.Value[5],
                    pisid->IdentifierAuthority.Value[4] ),
            MAKEWORD( pisid->IdentifierAuthority.Value[3],
                    pisid->IdentifierAuthority.Value[2] )
        ) );
    DumpString(buf, -1, pwptr, plen);

    for( i=0; i<pisid->SubAuthorityCount; i++ )
    {
        sprintfW( buf, subauthfmt, pisid->SubAuthority[i] );
        DumpString(buf, -1, pwptr, plen);
    }
    return TRUE;
}

static BOOL DumpSid(PSID psid, WCHAR **pwptr, ULONG *plen)
{
    size_t i;
    for (i = 0; i < sizeof(WellKnownSids) / sizeof(WellKnownSids[0]); i++)
    {
        if (WellKnownSids[i].wstr[0] && EqualSid(psid, (PSID)&(WellKnownSids[i].Sid.Revision)))
        {
            DumpString(WellKnownSids[i].wstr, 2, pwptr, plen);
            return TRUE;
        }
    }

    return DumpSidNumeric(psid, pwptr, plen);
}

static const LPCWSTR AceRightBitNames[32] = {
        SDDL_CREATE_CHILD,        /*  0 */
        SDDL_DELETE_CHILD,
        SDDL_LIST_CHILDREN,
        SDDL_SELF_WRITE,
        SDDL_READ_PROPERTY,       /*  4 */
        SDDL_WRITE_PROPERTY,
        SDDL_DELETE_TREE,
        SDDL_LIST_OBJECT,
        SDDL_CONTROL_ACCESS,      /*  8 */
        NULL,
        NULL,
        NULL,
        NULL,                     /* 12 */
        NULL,
        NULL,
        NULL,
        SDDL_STANDARD_DELETE,     /* 16 */
        SDDL_READ_CONTROL,
        SDDL_WRITE_DAC,
        SDDL_WRITE_OWNER,
        NULL,                     /* 20 */
        NULL,
        NULL,
        NULL,
        NULL,                     /* 24 */
        NULL,
        NULL,
        NULL,
        SDDL_GENERIC_ALL,         /* 28 */
        SDDL_GENERIC_EXECUTE,
        SDDL_GENERIC_WRITE,
        SDDL_GENERIC_READ
};

static void DumpRights(DWORD mask, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR fmtW[] = {'0','x','%','x',0};
    WCHAR buf[15];
    size_t i;

    if (mask == 0)
        return;

    /* first check if the right have name */
    for (i = 0; i < sizeof(AceRights)/sizeof(AceRights[0]); i++)
    {
        if (AceRights[i].wstr == NULL)
            break;
        if (mask == AceRights[i].value)
        {
            DumpString(AceRights[i].wstr, -1, pwptr, plen);
            return;
        }
    }

    /* then check if it can be built from bit names */
    for (i = 0; i < 32; i++)
    {
        if ((mask & (1 << i)) && (AceRightBitNames[i] == NULL))
        {
            /* can't be built from bit names */
            sprintfW(buf, fmtW, mask);
            DumpString(buf, -1, pwptr, plen);
            return;
        }
    }

    /* build from bit names */
    for (i = 0; i < 32; i++)
        if (mask & (1 << i))
            DumpString(AceRightBitNames[i], -1, pwptr, plen);
}

static BOOL DumpAce(LPVOID pace, WCHAR **pwptr, ULONG *plen)
{
    ACCESS_ALLOWED_ACE *piace; /* all the supported ACEs have the same memory layout */
    static const WCHAR openbr = '(';
    static const WCHAR closebr = ')';
    static const WCHAR semicolon = ';';

    if (((PACE_HEADER)pace)->AceType > SYSTEM_ALARM_ACE_TYPE || ((PACE_HEADER)pace)->AceSize < sizeof(ACCESS_ALLOWED_ACE))
    {
        SetLastError(ERROR_INVALID_ACL);
        return FALSE;
    }

    piace = pace;
    DumpString(&openbr, 1, pwptr, plen);
    switch (piace->Header.AceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
            DumpString(SDDL_ACCESS_ALLOWED, -1, pwptr, plen);
            break;
        case ACCESS_DENIED_ACE_TYPE:
            DumpString(SDDL_ACCESS_DENIED, -1, pwptr, plen);
            break;
        case SYSTEM_AUDIT_ACE_TYPE:
            DumpString(SDDL_AUDIT, -1, pwptr, plen);
            break;
        case SYSTEM_ALARM_ACE_TYPE:
            DumpString(SDDL_ALARM, -1, pwptr, plen);
            break;
    }
    DumpString(&semicolon, 1, pwptr, plen);

    if (piace->Header.AceFlags & OBJECT_INHERIT_ACE)
        DumpString(SDDL_OBJECT_INHERIT, -1, pwptr, plen);
    if (piace->Header.AceFlags & CONTAINER_INHERIT_ACE)
        DumpString(SDDL_CONTAINER_INHERIT, -1, pwptr, plen);
    if (piace->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE)
        DumpString(SDDL_NO_PROPAGATE, -1, pwptr, plen);
    if (piace->Header.AceFlags & INHERIT_ONLY_ACE)
        DumpString(SDDL_INHERIT_ONLY, -1, pwptr, plen);
    if (piace->Header.AceFlags & INHERITED_ACE)
        DumpString(SDDL_INHERITED, -1, pwptr, plen);
    if (piace->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG)
        DumpString(SDDL_AUDIT_SUCCESS, -1, pwptr, plen);
    if (piace->Header.AceFlags & FAILED_ACCESS_ACE_FLAG)
        DumpString(SDDL_AUDIT_FAILURE, -1, pwptr, plen);
    DumpString(&semicolon, 1, pwptr, plen);
    DumpRights(piace->Mask, pwptr, plen);
    DumpString(&semicolon, 1, pwptr, plen);
    /* objects not supported */
    DumpString(&semicolon, 1, pwptr, plen);
    /* objects not supported */
    DumpString(&semicolon, 1, pwptr, plen);
    if (!DumpSid((PSID)&piace->SidStart, pwptr, plen))
        return FALSE;
    DumpString(&closebr, 1, pwptr, plen);
    return TRUE;
}

static BOOL DumpAcl(PACL pacl, WCHAR **pwptr, ULONG *plen, BOOL protected, BOOL autoInheritReq, BOOL autoInherited)
{
    WORD count;
    int i;

    if (protected)
        DumpString(SDDL_PROTECTED, -1, pwptr, plen);
    if (autoInheritReq)
        DumpString(SDDL_AUTO_INHERIT_REQ, -1, pwptr, plen);
    if (autoInherited)
        DumpString(SDDL_AUTO_INHERITED, -1, pwptr, plen);

    if (pacl == NULL)
        return TRUE;

    if (!IsValidAcl(pacl))
        return FALSE;

    count = pacl->AceCount;
    for (i = 0; i < count; i++)
    {
        LPVOID ace;
        if (!GetAce(pacl, i, &ace))
            return FALSE;
        if (!DumpAce(ace, pwptr, plen))
            return FALSE;
    }

    return TRUE;
}

static BOOL DumpOwner(PSECURITY_DESCRIPTOR SecurityDescriptor, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR prefix[] = {'O',':',0};
    BOOL bDefaulted;
    PSID psid;

    if (!GetSecurityDescriptorOwner(SecurityDescriptor, &psid, &bDefaulted))
        return FALSE;

    if (psid == NULL)
        return TRUE;

    DumpString(prefix, -1, pwptr, plen);
    if (!DumpSid(psid, pwptr, plen))
        return FALSE;
    return TRUE;
}

static BOOL DumpGroup(PSECURITY_DESCRIPTOR SecurityDescriptor, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR prefix[] = {'G',':',0};
    BOOL bDefaulted;
    PSID psid;

    if (!GetSecurityDescriptorGroup(SecurityDescriptor, &psid, &bDefaulted))
        return FALSE;

    if (psid == NULL)
        return TRUE;

    DumpString(prefix, -1, pwptr, plen);
    if (!DumpSid(psid, pwptr, plen))
        return FALSE;
    return TRUE;
}

static BOOL DumpDacl(PSECURITY_DESCRIPTOR SecurityDescriptor, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR dacl[] = {'D',':',0};
    SECURITY_DESCRIPTOR_CONTROL control;
    BOOL present, defaulted;
    DWORD revision;
    PACL pacl;

    if (!GetSecurityDescriptorDacl(SecurityDescriptor, &present, &pacl, &defaulted))
        return FALSE;

    if (!GetSecurityDescriptorControl(SecurityDescriptor, &control, &revision))
        return FALSE;

    if (!present)
        return TRUE;

    DumpString(dacl, 2, pwptr, plen);
    if (!DumpAcl(pacl, pwptr, plen, control & SE_DACL_PROTECTED, control & SE_DACL_AUTO_INHERIT_REQ, control & SE_DACL_AUTO_INHERITED))
        return FALSE;
    return TRUE;
}

static BOOL DumpSacl(PSECURITY_DESCRIPTOR SecurityDescriptor, WCHAR **pwptr, ULONG *plen)
{
    static const WCHAR sacl[] = {'S',':',0};
    SECURITY_DESCRIPTOR_CONTROL control;
    BOOL present, defaulted;
    DWORD revision;
    PACL pacl;

    if (!GetSecurityDescriptorSacl(SecurityDescriptor, &present, &pacl, &defaulted))
        return FALSE;

    if (!GetSecurityDescriptorControl(SecurityDescriptor, &control, &revision))
        return FALSE;

    if (!present)
        return TRUE;

    DumpString(sacl, 2, pwptr, plen);
    if (!DumpAcl(pacl, pwptr, plen, control & SE_SACL_PROTECTED, control & SE_SACL_AUTO_INHERIT_REQ, control & SE_SACL_AUTO_INHERITED))
        return FALSE;
    return TRUE;
}

/******************************************************************************
 * ConvertSecurityDescriptorToStringSecurityDescriptorW [ADVAPI32.@]
 */
BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorW(PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD SDRevision, SECURITY_INFORMATION RequestedInformation, LPWSTR *OutputString, PULONG OutputLen)
{
    ULONG len;
    WCHAR *wptr, *wstr;

    if (SDRevision != SDDL_REVISION_1)
    {
        ERR("Program requested unknown SDDL revision %d\n", SDRevision);
        SetLastError(ERROR_UNKNOWN_REVISION);
        return FALSE;
    }

    len = 0;
    if (RequestedInformation & OWNER_SECURITY_INFORMATION)
        if (!DumpOwner(SecurityDescriptor, NULL, &len))
            return FALSE;
    if (RequestedInformation & GROUP_SECURITY_INFORMATION)
        if (!DumpGroup(SecurityDescriptor, NULL, &len))
            return FALSE;
    if (RequestedInformation & DACL_SECURITY_INFORMATION)
        if (!DumpDacl(SecurityDescriptor, NULL, &len))
            return FALSE;
    if (RequestedInformation & SACL_SECURITY_INFORMATION)
        if (!DumpSacl(SecurityDescriptor, NULL, &len))
            return FALSE;

    wstr = wptr = LocalAlloc(0, (len + 1)*sizeof(WCHAR));
#ifdef __REACTOS__
    if (wstr == NULL)
        return FALSE;
#endif

    if (RequestedInformation & OWNER_SECURITY_INFORMATION)
        if (!DumpOwner(SecurityDescriptor, &wptr, NULL)) {
            LocalFree (wstr);
            return FALSE;
        }
    if (RequestedInformation & GROUP_SECURITY_INFORMATION)
        if (!DumpGroup(SecurityDescriptor, &wptr, NULL)) {
            LocalFree (wstr);
            return FALSE;
        }
    if (RequestedInformation & DACL_SECURITY_INFORMATION)
        if (!DumpDacl(SecurityDescriptor, &wptr, NULL)) {
            LocalFree (wstr);
            return FALSE;
        }
    if (RequestedInformation & SACL_SECURITY_INFORMATION)
        if (!DumpSacl(SecurityDescriptor, &wptr, NULL)) {
            LocalFree (wstr);
            return FALSE;
        }
    *wptr = 0;

    TRACE("ret: %s, %d\n", wine_dbgstr_w(wstr), len);
    *OutputString = wstr;
    if (OutputLen)
        *OutputLen = strlenW(*OutputString)+1;
    return TRUE;
}

/******************************************************************************
 * ConvertSecurityDescriptorToStringSecurityDescriptorA [ADVAPI32.@]
 */
BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorA(PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD SDRevision, SECURITY_INFORMATION Information, LPSTR *OutputString, PULONG OutputLen)
{
    LPWSTR wstr;
    ULONG len;
    if (ConvertSecurityDescriptorToStringSecurityDescriptorW(SecurityDescriptor, SDRevision, Information, &wstr, &len))
    {
        int lenA;

        lenA = WideCharToMultiByte(CP_ACP, 0, wstr, len, NULL, 0, NULL, NULL);
        *OutputString = heap_alloc(lenA);
#ifdef __REACTOS__
        if (*OutputString == NULL)
        {
            LocalFree(wstr);
            *OutputLen = 0;
            return FALSE;
        }
#endif
        WideCharToMultiByte(CP_ACP, 0, wstr, len, *OutputString, lenA, NULL, NULL);
        LocalFree(wstr);

        if (OutputLen != NULL)
            *OutputLen = lenA;
        return TRUE;
    }
    else
    {
        *OutputString = NULL;
        if (OutputLen)
            *OutputLen = 0;
        return FALSE;
    }
}

/******************************************************************************
 * ConvertStringSidToSidW [ADVAPI32.@]
 */
BOOL WINAPI ConvertStringSidToSidW(LPCWSTR StringSid, PSID* Sid)
{
    BOOL bret = FALSE;
    DWORD cBytes;

    TRACE("%s, %p\n", debugstr_w(StringSid), Sid);
    if (GetVersion() & 0x80000000)
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    else if (!StringSid || !Sid)
        SetLastError(ERROR_INVALID_PARAMETER);
    else if (ParseStringSidToSid(StringSid, NULL, &cBytes))
    {
        PSID pSid = *Sid = LocalAlloc(0, cBytes);

        bret = ParseStringSidToSid(StringSid, pSid, &cBytes);
        if (!bret)
            LocalFree(*Sid); 
    }
    return bret;
}

/******************************************************************************
 * ConvertStringSidToSidA [ADVAPI32.@]
 */
BOOL WINAPI ConvertStringSidToSidA(LPCSTR StringSid, PSID* Sid)
{
    BOOL bret = FALSE;

    TRACE("%s, %p\n", debugstr_a(StringSid), Sid);
    if (GetVersion() & 0x80000000)
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    else if (!StringSid || !Sid)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        WCHAR *wStringSid = SERV_dup(StringSid);
        bret = ConvertStringSidToSidW(wStringSid, Sid);
        heap_free(wStringSid);
    }
    return bret;
}

/*
 * @implemented
 */
BOOL
WINAPI
ConvertSidToStringSidW(PSID Sid,
                       LPWSTR *StringSid)
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WCHAR FixedBuffer[64];

    if (!RtlValidSid(Sid))
    {
        SetLastError(ERROR_INVALID_SID);
        return FALSE;
    }

    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = sizeof(FixedBuffer);
    UnicodeString.Buffer = FixedBuffer;
    Status = RtlConvertSidToUnicodeString(&UnicodeString, Sid, FALSE);
    if (STATUS_BUFFER_TOO_SMALL == Status)
    {
        Status = RtlConvertSidToUnicodeString(&UnicodeString, Sid, TRUE);
    }

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    *StringSid = LocalAlloc(LMEM_FIXED, UnicodeString.Length + sizeof(WCHAR));
    if (NULL == *StringSid)
    {
        if (UnicodeString.Buffer != FixedBuffer)
        {
            RtlFreeUnicodeString(&UnicodeString);
        }
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

    MoveMemory(*StringSid, UnicodeString.Buffer, UnicodeString.Length);
    ZeroMemory((PCHAR) *StringSid + UnicodeString.Length, sizeof(WCHAR));
    if (UnicodeString.Buffer != FixedBuffer)
    {
        RtlFreeUnicodeString(&UnicodeString);
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ConvertSidToStringSidA(PSID Sid,
                       LPSTR *StringSid)
{
    LPWSTR StringSidW;
    int Len;

    if (!ConvertSidToStringSidW(Sid, &StringSidW))
    {
        return FALSE;
    }

    Len = WideCharToMultiByte(CP_ACP, 0, StringSidW, -1, NULL, 0, NULL, NULL);
    if (Len <= 0)
    {
        LocalFree(StringSidW);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    *StringSid = LocalAlloc(LMEM_FIXED, Len);
    if (NULL == *StringSid)
    {
        LocalFree(StringSidW);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, StringSidW, -1, *StringSid, Len, NULL, NULL))
    {
        LocalFree(StringSid);
        LocalFree(StringSidW);
        return FALSE;
    }

    LocalFree(StringSidW);

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
CreateProcessWithLogonW(LPCWSTR lpUsername,
                        LPCWSTR lpDomain,
                        LPCWSTR lpPassword,
                        DWORD dwLogonFlags,
                        LPCWSTR lpApplicationName,
                        LPWSTR lpCommandLine,
                        DWORD dwCreationFlags,
                        LPVOID lpEnvironment,
                        LPCWSTR lpCurrentDirectory,
                        LPSTARTUPINFOW lpStartupInfo,
                        LPPROCESS_INFORMATION lpProcessInformation)
{
    FIXME("%s %s %s 0x%08x %s %s 0x%08x %p %s %p %p stub\n", debugstr_w(lpUsername), debugstr_w(lpDomain),
    debugstr_w(lpPassword), dwLogonFlags, debugstr_w(lpApplicationName),
    debugstr_w(lpCommandLine), dwCreationFlags, lpEnvironment, debugstr_w(lpCurrentDirectory),
    lpStartupInfo, lpProcessInformation);

    return FALSE;
}

BOOL WINAPI CreateProcessWithTokenW(HANDLE token, DWORD logon_flags, LPCWSTR application_name, LPWSTR command_line,
        DWORD creation_flags, void *environment, LPCWSTR current_directory, STARTUPINFOW *startup_info,
        PROCESS_INFORMATION *process_information )
{
    FIXME("%p 0x%08x %s %s 0x%08x %p %s %p %p - semi-stub\n", token,
          logon_flags, debugstr_w(application_name), debugstr_w(command_line),
          creation_flags, environment, debugstr_w(current_directory),
          startup_info, process_information);

    /* FIXME: check if handles should be inherited */
    return CreateProcessW( application_name, command_line, NULL, NULL, FALSE, creation_flags, environment,
                           current_directory, startup_info, process_information );
}

/*
 * @implemented
 */
BOOL WINAPI
DuplicateTokenEx(IN HANDLE ExistingTokenHandle,
                 IN DWORD dwDesiredAccess,
                 IN LPSECURITY_ATTRIBUTES lpTokenAttributes  OPTIONAL,
                 IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                 IN TOKEN_TYPE TokenType,
                 OUT PHANDLE DuplicateTokenHandle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    SECURITY_QUALITY_OF_SERVICE Sqos;

    TRACE("%p 0x%08x 0x%08x 0x%08x %p\n", ExistingTokenHandle, dwDesiredAccess,
        ImpersonationLevel, TokenType, DuplicateTokenHandle);

    Sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Sqos.ImpersonationLevel = ImpersonationLevel;
    Sqos.ContextTrackingMode = 0;
    Sqos.EffectiveOnly = FALSE;

    if (lpTokenAttributes != NULL)
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   lpTokenAttributes->bInheritHandle ? OBJ_INHERIT : 0,
                                   NULL,
                                   lpTokenAttributes->lpSecurityDescriptor);
    }
    else
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
    }

    ObjectAttributes.SecurityQualityOfService = &Sqos;

    Status = NtDuplicateToken(ExistingTokenHandle,
                              dwDesiredAccess,
                              &ObjectAttributes,
                              FALSE,
                              TokenType,
                              DuplicateTokenHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtDuplicateToken failed: Status %08x\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    TRACE("Returning token %p.\n", *DuplicateTokenHandle);

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
DuplicateToken(IN HANDLE ExistingTokenHandle,
               IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
               OUT PHANDLE DuplicateTokenHandle)
{
    return DuplicateTokenEx(ExistingTokenHandle,
                            TOKEN_IMPERSONATE | TOKEN_QUERY,
                            NULL,
                            ImpersonationLevel,
                            TokenImpersonation,
                            DuplicateTokenHandle);
}

/******************************************************************************
 * ComputeStringSidSize
 */
static DWORD ComputeStringSidSize(LPCWSTR StringSid)
{
    if (StringSid[0] == 'S' && StringSid[1] == '-') /* S-R-I(-S)+ */
    {
        int ctok = 0;
        while (*StringSid)
        {
            if (*StringSid == '-')
                ctok++;
            StringSid++;
        }

        if (ctok >= 3)
            return GetSidLengthRequired(ctok - 2);
    }
    else /* String constant format  - Only available in winxp and above */
    {
        unsigned int i;

        for (i = 0; i < sizeof(WellKnownSids)/sizeof(WellKnownSids[0]); i++)
            if (!strncmpW(WellKnownSids[i].wstr, StringSid, 2))
                return GetSidLengthRequired(WellKnownSids[i].Sid.SubAuthorityCount);

        for (i = 0; i < sizeof(WellKnownRids)/sizeof(WellKnownRids[0]); i++)
            if (!strncmpW(WellKnownRids[i].wstr, StringSid, 2))
            {
                MAX_SID local;
                ADVAPI_GetComputerSid(&local);
                return GetSidLengthRequired(*GetSidSubAuthorityCount(&local) + 1);
            }

    }

    return GetSidLengthRequired(0);
}

/******************************************************************************
 * ParseStringSidToSid
 */
static BOOL ParseStringSidToSid(LPCWSTR StringSid, PSID pSid, LPDWORD cBytes)
{
    BOOL bret = FALSE;
    SID* pisid=pSid;

    TRACE("%s, %p, %p\n", debugstr_w(StringSid), pSid, cBytes);
    if (!StringSid)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TRACE("StringSid is NULL, returning FALSE\n");
        return FALSE;
    }

    while (*StringSid == ' ')
        StringSid++;

    if (!*StringSid)
        goto lend; /* ERROR_INVALID_SID */

    *cBytes = ComputeStringSidSize(StringSid);
    if (!pisid) /* Simply compute the size */
    {
        TRACE("only size requested, returning TRUE with %d\n", *cBytes);
        return TRUE;
    }

    if (StringSid[0] == 'S' && StringSid[1] == '-') /* S-R-I-S-S */
    {
        DWORD i = 0, identAuth;
        DWORD csubauth = ((*cBytes - GetSidLengthRequired(0)) / sizeof(DWORD));

        StringSid += 2; /* Advance to Revision */
        pisid->Revision = atoiW(StringSid);

        if (pisid->Revision != SDDL_REVISION)
        {
            TRACE("Revision %d is unknown\n", pisid->Revision);
            goto lend; /* ERROR_INVALID_SID */
        }
        if (csubauth == 0)
        {
            TRACE("SubAuthorityCount is 0\n");
            goto lend; /* ERROR_INVALID_SID */
        }

        pisid->SubAuthorityCount = csubauth;

        /* Advance to identifier authority */
        while (*StringSid && *StringSid != '-')
            StringSid++;
        if (*StringSid == '-')
            StringSid++;

        /* MS' implementation can't handle values greater than 2^32 - 1, so
         * we don't either; assume most significant bytes are always 0
         */
        pisid->IdentifierAuthority.Value[0] = 0;
        pisid->IdentifierAuthority.Value[1] = 0;
        identAuth = atoiW(StringSid);
        pisid->IdentifierAuthority.Value[5] = identAuth & 0xff;
        pisid->IdentifierAuthority.Value[4] = (identAuth & 0xff00) >> 8;
        pisid->IdentifierAuthority.Value[3] = (identAuth & 0xff0000) >> 16;
        pisid->IdentifierAuthority.Value[2] = (identAuth & 0xff000000) >> 24;

        /* Advance to first sub authority */
        while (*StringSid && *StringSid != '-')
            StringSid++;
        if (*StringSid == '-')
            StringSid++;

        while (*StringSid)
        {
            pisid->SubAuthority[i++] = atoiW(StringSid);

            while (*StringSid && *StringSid != '-')
                StringSid++;
            if (*StringSid == '-')
                StringSid++;
        }

        if (i != pisid->SubAuthorityCount)
            goto lend; /* ERROR_INVALID_SID */

        bret = TRUE;
    }
    else /* String constant format  - Only available in winxp and above */
    {
        unsigned int i;
        pisid->Revision = SDDL_REVISION;

        for (i = 0; i < sizeof(WellKnownSids)/sizeof(WellKnownSids[0]); i++)
            if (!strncmpW(WellKnownSids[i].wstr, StringSid, 2))
            {
                DWORD j;
                pisid->SubAuthorityCount = WellKnownSids[i].Sid.SubAuthorityCount;
                pisid->IdentifierAuthority = WellKnownSids[i].Sid.IdentifierAuthority;
                for (j = 0; j < WellKnownSids[i].Sid.SubAuthorityCount; j++)
                    pisid->SubAuthority[j] = WellKnownSids[i].Sid.SubAuthority[j];
                bret = TRUE;
            }

        for (i = 0; i < sizeof(WellKnownRids)/sizeof(WellKnownRids[0]); i++)
            if (!strncmpW(WellKnownRids[i].wstr, StringSid, 2))
            {
                ADVAPI_GetComputerSid(pisid);
                pisid->SubAuthority[pisid->SubAuthorityCount] = WellKnownRids[i].Rid;
                pisid->SubAuthorityCount++;
                bret = TRUE;
            }

        if (!bret)
            FIXME("String constant not supported: %s\n", debugstr_wn(StringSid, 2));
    }

lend:
    if (!bret)
        SetLastError(ERROR_INVALID_SID);

    TRACE("returning %s\n", bret ? "TRUE" : "FALSE");
    return bret;
}

/**********************************************************************
 * GetNamedSecurityInfoA			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
GetNamedSecurityInfoA(LPSTR pObjectName,
                      SE_OBJECT_TYPE ObjectType,
                      SECURITY_INFORMATION SecurityInfo,
                      PSID *ppsidOwner,
                      PSID *ppsidGroup,
                      PACL *ppDacl,
                      PACL *ppSacl,
                      PSECURITY_DESCRIPTOR *ppSecurityDescriptor)
{
    DWORD len;
    LPWSTR wstr = NULL;
    DWORD r;

    TRACE("%s %d %d %p %p %p %p %p\n", pObjectName, ObjectType, SecurityInfo,
        ppsidOwner, ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor);

    if( pObjectName )
    {
        len = MultiByteToWideChar( CP_ACP, 0, pObjectName, -1, NULL, 0 );
        wstr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar( CP_ACP, 0, pObjectName, -1, wstr, len );
    }

    r = GetNamedSecurityInfoW( wstr, ObjectType, SecurityInfo, ppsidOwner,
                           ppsidGroup, ppDacl, ppSacl, ppSecurityDescriptor );

    HeapFree( GetProcessHeap(), 0, wstr );

    return r;
}

/******************************************************************************
 * GetWindowsAccountDomainSid         [ADVAPI32.@]
 */
BOOL WINAPI GetWindowsAccountDomainSid( PSID sid, PSID domain_sid, DWORD *size )
{
    SID_IDENTIFIER_AUTHORITY domain_ident = { SECURITY_NT_AUTHORITY };
    DWORD required_size;
    int i;

    FIXME( "(%p %p %p): semi-stub\n", sid, domain_sid, size );

    if (!sid || !IsValidSid( sid ))
    {
        SetLastError( ERROR_INVALID_SID );
        return FALSE;
    }

    if (!size)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (*GetSidSubAuthorityCount( sid ) < 4)
    {
        SetLastError( ERROR_INVALID_SID );
        return FALSE;
    }

    required_size = GetSidLengthRequired( 4 );
    if (*size < required_size || !domain_sid)
    {
        *size = required_size;
        SetLastError( domain_sid ? ERROR_INSUFFICIENT_BUFFER :
                                   ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    InitializeSid( domain_sid, &domain_ident, 4 );
    for (i = 0; i < 4; i++)
        *GetSidSubAuthority( domain_sid, i ) = *GetSidSubAuthority( sid, i );

    *size = required_size;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
EqualDomainSid(IN PSID pSid1,
               IN PSID pSid2,
               OUT BOOL* pfEqual)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
