/*
 * COPYRIGHT:       See COPYING in the top level directory
 * WINE COPYRIGHT:
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
 * Copyright 2006 Hervé Poussineau
 *
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sid.c
 * PURPOSE:         Security ID functions
 */

#include <advapi32.h>
#include <sddl.h>
#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

#define MAX_GUID_STRING_LEN 39

BOOL STDCALL
AddAuditAccessAceEx(PACL pAcl,
		    DWORD dwAceRevision,
		    DWORD AceFlags,
		    DWORD dwAccessMask,
		    PSID pSid,
		    BOOL bAuditSuccess,
		    BOOL bAuditFailure);

typedef struct RECORD
{
	LPCWSTR key;
	DWORD value;
} RECORD;


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
};

static const SID sidWorld = { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY} , { SECURITY_WORLD_RID } };

/*
 * ACE types
 */
static const WCHAR SDDL_ACCESS_ALLOWED[]        = {'A',0};
static const WCHAR SDDL_ACCESS_DENIED[]         = {'D',0};
static const WCHAR SDDL_OBJECT_ACCESS_ALLOWED[] = {'O','A',0};
static const WCHAR SDDL_OBJECT_ACCESS_DENIED[]  = {'O','D',0};
static const WCHAR SDDL_AUDIT[]                 = {'A','U',0};
static const WCHAR SDDL_ALARM[]                 = {'A','L',0};
static const WCHAR SDDL_OBJECT_AUDIT[]          = {'O','U',0};
static const WCHAR SDDL_OBJECT_ALARM[]          = {'O','L',0};

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
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

static BOOL
FindKeyInTable(
	IN const RECORD* Table,
	IN LPCWSTR Key,
	OUT SIZE_T* pKeyLength,
	OUT DWORD* pItem)
{
	const RECORD* pRecord = Table;
	while (pRecord->key != NULL)
	{
		if (wcsncmp(pRecord->key, Key, wcslen(pRecord->key)) == 0)
		{
			*pKeyLength = wcslen(pRecord->key);
			*pItem = pRecord->value;
			return TRUE;
		}
		pRecord++;
	}
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
}

static BOOL
ParseSidString(
	IN LPCWSTR Buffer,
	OUT PSID* pSid,
	OUT SIZE_T* pLength)
{
	WCHAR str[SDDL_ALIAS_SIZE + 1];
	LPWSTR strSid;
	LPCWSTR end;
	BOOL ret;
	DWORD i;

	wcsncpy(str, Buffer, SDDL_ALIAS_SIZE);
	for (i = SDDL_ALIAS_SIZE; i > 0; i--)
	{
		str[i] = UNICODE_NULL;
		if (ConvertStringSidToSidW(str, pSid))
		{
			*pLength = i;
			return TRUE;
		}
	}

	end = wcschr(Buffer, SDDL_ACE_ENDC);
	if (!end)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	strSid = (LPWSTR)LocalAlloc(0, (end - Buffer) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
	if (!strSid)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	wcsncpy(strSid, Buffer, end - Buffer + 1);
	strSid[end - Buffer] = UNICODE_NULL;
	*pLength = end - Buffer;
	ret = ConvertStringSidToSidW(strSid, pSid);
	LocalFree(strSid);
	return ret;
}

static const RECORD DaclFlagTable[] =
{
	{ SDDL_PROTECTED, SE_DACL_PROTECTED },
	{ SDDL_AUTO_INHERIT_REQ, SE_DACL_AUTO_INHERIT_REQ },
	{ SDDL_AUTO_INHERITED, SE_DACL_AUTO_INHERITED },
	{ NULL, 0 },
};

static const RECORD SaclFlagTable[] =
{
	{ SDDL_PROTECTED, SE_SACL_PROTECTED },
	{ SDDL_AUTO_INHERIT_REQ, SE_SACL_AUTO_INHERIT_REQ },
	{ SDDL_AUTO_INHERITED, SE_SACL_AUTO_INHERITED },
	{ NULL, 0 },
};

static const RECORD AceFlagTable[] =
{
	{ SDDL_CONTAINER_INHERIT, CONTAINER_INHERIT_ACE },
	{ SDDL_OBJECT_INHERIT, OBJECT_INHERIT_ACE },
	{ SDDL_NO_PROPAGATE, NO_PROPAGATE_INHERIT_ACE },
	{ SDDL_INHERIT_ONLY, INHERIT_ONLY_ACE },
	{ SDDL_INHERITED, INHERITED_ACE },
	{ SDDL_AUDIT_SUCCESS, SUCCESSFUL_ACCESS_ACE_FLAG },
	{ SDDL_AUDIT_FAILURE, FAILED_ACCESS_ACE_FLAG },
	{ NULL, 0 },
};

static BOOL
ParseFlagsString(
	IN LPCWSTR Buffer,
	IN const RECORD* FlagTable,
	IN WCHAR LimitChar,
	OUT DWORD* pFlags,
	OUT SIZE_T* pLength)
{
	LPCWSTR ptr = Buffer;
	SIZE_T PartialLength;
	DWORD Flag;

	*pFlags = 0;
	while (*ptr != LimitChar)
	{
		if (!FindKeyInTable(FlagTable, ptr, &PartialLength, &Flag))
			return FALSE;
		*pFlags |= Flag;
		ptr += PartialLength;
	}
	*pLength = ptr - Buffer;
	return TRUE;
}

static const RECORD AccessMaskTable[] =
{
	{ SDDL_GENERIC_ALL, GENERIC_ALL },
	{ SDDL_GENERIC_READ, GENERIC_READ },
	{ SDDL_GENERIC_WRITE, GENERIC_WRITE },
	{ SDDL_GENERIC_EXECUTE, GENERIC_EXECUTE },
	{ SDDL_READ_CONTROL, READ_CONTROL },
	{ SDDL_STANDARD_DELETE, DELETE },
	{ SDDL_WRITE_DAC, WRITE_DAC },
	{ SDDL_WRITE_OWNER, WRITE_OWNER },
	{ SDDL_READ_PROPERTY, ADS_RIGHT_DS_READ_PROP },
	{ SDDL_WRITE_PROPERTY, ADS_RIGHT_DS_WRITE_PROP },
	{ SDDL_CREATE_CHILD, ADS_RIGHT_DS_CREATE_CHILD },
	{ SDDL_DELETE_CHILD, ADS_RIGHT_DS_DELETE_CHILD },
	{ SDDL_LIST_CHILDREN, ADS_RIGHT_ACTRL_DS_LIST },
	{ SDDL_SELF_WRITE, ADS_RIGHT_DS_SELF },
	{ SDDL_LIST_OBJECT, ADS_RIGHT_DS_LIST_OBJECT },
	{ SDDL_DELETE_TREE, ADS_RIGHT_DS_DELETE_TREE },
	{ SDDL_CONTROL_ACCESS, ADS_RIGHT_DS_CONTROL_ACCESS },
	{ SDDL_FILE_ALL, FILE_ALL_ACCESS },
	{ SDDL_FILE_READ, FILE_GENERIC_READ },
	{ SDDL_FILE_WRITE, FILE_GENERIC_WRITE },
	{ SDDL_FILE_EXECUTE, FILE_GENERIC_EXECUTE },
	{ SDDL_KEY_ALL, KEY_ALL_ACCESS },
	{ SDDL_KEY_READ, KEY_READ },
	{ SDDL_KEY_WRITE, KEY_WRITE },
	{ SDDL_KEY_EXECUTE, KEY_EXECUTE },
	{ NULL, 0 },
};

static BOOL
ParseAccessMaskString(
	IN LPCWSTR Buffer,
	OUT DWORD* pAccessMask,
	OUT SIZE_T* pLength)
{
	/* FIXME: Allow hexadecimal string for access rights! */

	return ParseFlagsString(Buffer, AccessMaskTable, SDDL_SEPERATORC, pAccessMask, pLength);
}

static BOOL
ParseGuidString(
	IN LPCWSTR Buffer,
	OUT GUID* pGuid,
	OUT BOOL* pIsGuidValid,
	OUT SIZE_T* pLength)
{
	WCHAR GuidStr[MAX_GUID_STRING_LEN + 1];
	LPCWSTR end;

	end = wcschr(Buffer, SDDL_SEPERATORC);
	if (!end)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	*pLength = end - Buffer;
	*pIsGuidValid = (end != Buffer);
	if (!*pIsGuidValid)
		return TRUE;

	if (end - Buffer > MAX_GUID_STRING_LEN - 1)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	GuidStr[end - Buffer] = UNICODE_NULL;
	wcsncpy(GuidStr, Buffer, end - Buffer);
	if (RPC_S_OK != UuidFromStringW((unsigned short*)&GuidStr, pGuid))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	return TRUE;
}

static const RECORD AceTypeTable[] =
{
	{ SDDL_OBJECT_ACCESS_ALLOWED, ACCESS_ALLOWED_OBJECT_ACE_TYPE },
	{ SDDL_OBJECT_ACCESS_DENIED, ACCESS_DENIED_OBJECT_ACE_TYPE },
	{ SDDL_AUDIT, SYSTEM_AUDIT_ACE_TYPE },
	{ SDDL_ALARM, SYSTEM_ALARM_ACE_TYPE },
	{ SDDL_OBJECT_AUDIT, SYSTEM_AUDIT_OBJECT_ACE_TYPE },
	{ SDDL_OBJECT_ALARM, SYSTEM_ALARM_OBJECT_ACE_TYPE },
	{ SDDL_ACCESS_ALLOWED, ACCESS_ALLOWED_ACE_TYPE },
	{ SDDL_ACCESS_DENIED, ACCESS_DENIED_ACE_TYPE },
	{ NULL, 0 },
};

static BOOL
ParseAceString(
	IN LPCWSTR Buffer,
	IN PACL pAcl,
	OUT SIZE_T* pLength)
{
	LPCWSTR ptr = Buffer;
	SIZE_T PartialLength;
	DWORD aceType, aceFlags, accessMask;
	GUID object, inheritObject;
	BOOL objectValid, inheritObjectValid;
	PSID sid = NULL;
	BOOL ret;

	if (*ptr != SDDL_ACE_BEGINC)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	ptr++; /* Skip SDDL_ACE_BEGINC */

	if (!FindKeyInTable(AceTypeTable, ptr, &PartialLength, &aceType))
		return FALSE;
	ptr += PartialLength;

	if (*ptr != SDDL_SEPERATORC)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	ptr++; /* Skip SDDL_SEPERATORC */

	if (!ParseFlagsString(ptr, AceFlagTable, SDDL_SEPERATORC, &aceFlags, &PartialLength))
		return FALSE;
	ptr += PartialLength + 1;

	if (!ParseAccessMaskString(ptr, &accessMask, &PartialLength))
		return FALSE;
	ptr += PartialLength + 1;

	if (!ParseGuidString(ptr, &object, &objectValid, &PartialLength))
		return FALSE;
	ptr += PartialLength + 1;

	if (!ParseGuidString(ptr, &inheritObject, &inheritObjectValid, &PartialLength))
		return FALSE;
	ptr += PartialLength + 1;

	if (!ParseSidString(ptr, &sid, &PartialLength))
		return FALSE;
	ptr += PartialLength;
	if (*ptr != SDDL_ACE_ENDC)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	ptr++; /* Skip SDDL_ACE_ENDC */
	*pLength = ptr - Buffer;

	switch (aceType)
	{
		case ACCESS_ALLOWED_ACE_TYPE:
			ret = AddAccessAllowedAceEx(
				pAcl,
				ACL_REVISION_DS,
				aceFlags,
				accessMask,
				sid);
			break;
		case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
			ret = AddAccessAllowedObjectAce(
				pAcl,
				ACL_REVISION_DS,
				aceFlags,
				accessMask,
				objectValid ? &object : NULL,
				inheritObjectValid ? &inheritObject : NULL,
				sid);
			break;
		case ACCESS_DENIED_ACE_TYPE:
			ret = AddAccessDeniedAceEx(
				pAcl,
				ACL_REVISION_DS,
				aceFlags,
				accessMask,
				sid);
			break;
		case ACCESS_DENIED_OBJECT_ACE_TYPE:
			ret = AddAccessDeniedObjectAce(
				pAcl,
				ACL_REVISION_DS,
				aceFlags,
				accessMask,
				objectValid ? &object : NULL,
				inheritObjectValid ? &inheritObject : NULL,
				sid);
			break;
		case SYSTEM_AUDIT_ACE_TYPE:
			ret = AddAuditAccessAceEx(
				pAcl,
				ACL_REVISION_DS,
				aceFlags,
				accessMask,
				sid,
				FALSE,
				FALSE);
			break;
		case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
			ret = AddAuditAccessObjectAce(
				pAcl,
				ACL_REVISION_DS,
				aceFlags,
				accessMask,
				objectValid ? &object : NULL,
				inheritObjectValid ? &inheritObject : NULL,
				sid,
				FALSE,
				FALSE);
			break;
		case SYSTEM_ALARM_ACE_TYPE:
		case SYSTEM_ALARM_OBJECT_ACE_TYPE:
		default:
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			ret = FALSE;
		}
	}
	LocalFree(sid);
	return ret;
}

/* Exported functions */

/*
 * @implemented
 */
BOOL STDCALL
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


/*
 * @implemented
 */
BOOL STDCALL
AllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
                         BYTE nSubAuthorityCount,
                         DWORD dwSubAuthority0,
                         DWORD dwSubAuthority1,
                         DWORD dwSubAuthority2,
                         DWORD dwSubAuthority3,
                         DWORD dwSubAuthority4,
                         DWORD dwSubAuthority5,
                         DWORD dwSubAuthority6,
                         DWORD dwSubAuthority7,
                         PSID *pSid)
{
    NTSTATUS Status;

    Status = RtlAllocateAndInitializeSid(pIdentifierAuthority,
                                         nSubAuthorityCount,
                                         dwSubAuthority0,
                                         dwSubAuthority1,
                                         dwSubAuthority2,
                                         dwSubAuthority3,
                                         dwSubAuthority4,
                                         dwSubAuthority5,
                                         dwSubAuthority6,
                                         dwSubAuthority7,
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
BOOL STDCALL
CopySid(DWORD nDestinationSidLength,
        PSID pDestinationSid,
        PSID pSourceSid)
{
    NTSTATUS Status;

    Status = RtlCopySid(nDestinationSidLength,
                        pDestinationSid,
                        pSourceSid);
    if (!NT_SUCCESS (Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

  return TRUE;
}


/******************************************************************************
 * ConvertSecurityDescriptorToStringSecurityDescriptorW [ADVAPI32.@]
 * @unimplemented
 */
BOOL WINAPI
ConvertSecurityDescriptorToStringSecurityDescriptorW(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                                     DWORD SDRevision,
                                                     SECURITY_INFORMATION SecurityInformation,
                                                     LPWSTR *OutputString,
                                                     PULONG OutputLen)
{
    if (SDRevision != SDDL_REVISION_1)
    {
        ERR("Pogram requested unknown SDDL revision %d\n", SDRevision);
        SetLastError(ERROR_UNKNOWN_REVISION);
        return FALSE;
    }

    FIXME("%s() not implemented!\n", __FUNCTION__);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/******************************************************************************
 * ConvertSecurityDescriptorToStringSecurityDescriptorA [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI
ConvertSecurityDescriptorToStringSecurityDescriptorA(PSECURITY_DESCRIPTOR SecurityDescriptor,
                                                     DWORD SDRevision,
                                                     SECURITY_INFORMATION Information,
                                                     LPSTR *OutputString,
                                                     PULONG OutputLen)
{
    LPWSTR wstr;
    ULONG len;
    if (ConvertSecurityDescriptorToStringSecurityDescriptorW(SecurityDescriptor, SDRevision, Information, &wstr, &len))
    {
        int lenA;

        lenA = WideCharToMultiByte(CP_ACP, 0, wstr, len, NULL, 0, NULL, NULL);
        *OutputString = HeapAlloc(GetProcessHeap(), 0, lenA);
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
 * ConvertStringSecurityDescriptorToSecurityDescriptorW [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI
ConvertStringSecurityDescriptorToSecurityDescriptorW(
	IN LPCWSTR StringSecurityDescriptor,
	IN DWORD StringSDRevision,
	OUT PSECURITY_DESCRIPTOR* SecurityDescriptor,
	OUT PULONG SecurityDescriptorSize)
{
	PSECURITY_DESCRIPTOR sd = NULL;
	BOOL ret = FALSE;

	if (!StringSecurityDescriptor)
		SetLastError(ERROR_INVALID_PARAMETER);
	else if (StringSDRevision != SDDL_REVISION_1)
		SetLastError(ERROR_INVALID_PARAMETER);
	else
	{
		LPCWSTR ptr = StringSecurityDescriptor;
		DWORD numberOfAces = 0;
		DWORD relativeSdSize;
		SIZE_T MaxAclSize;
		PSECURITY_DESCRIPTOR relativeSd = NULL;
		PSID pSid;
		PACL pAcl;
		BOOL present, dummy;
		/* An easy way to know how much space we need for an ACL is to count
		 * the number of ACEs and say that we have 1 SID by ACE
		 */
		ptr = wcschr(StringSecurityDescriptor, SDDL_ACE_BEGINC);
		while (ptr != NULL)
		{
			numberOfAces++;
			ptr = wcschr(ptr + 1, SDDL_ACE_BEGINC);
		}
		MaxAclSize = sizeof(ACL) + numberOfAces *
			(sizeof(ACCESS_ALLOWED_OBJECT_ACE) + SECURITY_MAX_SID_SIZE);

		sd = (SECURITY_DESCRIPTOR*)LocalAlloc(0, sizeof(SECURITY_DESCRIPTOR));
		if (!sd)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			return FALSE;
		}
		ret = InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
		if (!ret)
			goto cleanup;

		/* Now, really parse the string */
		ptr = StringSecurityDescriptor;
		while (*ptr)
		{
			if (ptr[1] != SDDL_DELIMINATORC)
			{
				SetLastError(ERROR_INVALID_PARAMETER);
				ret = FALSE;
				goto cleanup;
			}
			ptr += 2;
			switch (ptr[-2])
			{
				case 'O':
				case 'G':
				{
					PSID pSid;
					SIZE_T Length;

					ret = ParseSidString(ptr, &pSid, &Length);
					if (!ret)
						goto cleanup;
					if (ptr[-2] == 'O')
						ret = SetSecurityDescriptorOwner(sd, pSid, FALSE);
					else
						ret = SetSecurityDescriptorGroup(sd, pSid, FALSE);
					if (!ret)
					{
						LocalFree(pSid);
						goto cleanup;
					}
					ptr += Length;
					break;
				}
				case 'D':
				case 'S':
				{
					DWORD aclFlags;
					SIZE_T Length;
					BOOL isDacl = (ptr[-2] == 'D');

					if (isDacl)
						ret = ParseFlagsString(ptr, DaclFlagTable, SDDL_ACE_BEGINC, &aclFlags, &Length);
					else
						ret = ParseFlagsString(ptr, SaclFlagTable, SDDL_ACE_BEGINC, &aclFlags, &Length);
					if (!ret)
						goto cleanup;
					pAcl = (PACL)LocalAlloc(0, MaxAclSize);
					if (!pAcl)
					{
						SetLastError(ERROR_NOT_ENOUGH_MEMORY);
						ret = FALSE;
						goto cleanup;
					}
					if (!InitializeAcl(pAcl, (DWORD)MaxAclSize, ACL_REVISION_DS))
					{
						LocalFree(pAcl);
						goto cleanup;
					}
					if (aclFlags != 0)
					{
						ret = SetSecurityDescriptorControl(
							sd,
							(SECURITY_DESCRIPTOR_CONTROL)aclFlags,
							(SECURITY_DESCRIPTOR_CONTROL)aclFlags);
						if (!ret)
						{
							LocalFree(pAcl);
							goto cleanup;
						}
					}
					ptr += Length;
					while (*ptr == SDDL_ACE_BEGINC)
					{
						ret = ParseAceString(ptr, pAcl, &Length);
						if (!ret)
						{
							LocalFree(pAcl);
							goto cleanup;
						}
						ptr += Length;
					}
					if (isDacl)
						ret = SetSecurityDescriptorDacl(sd, TRUE, pAcl, FALSE);
					else
						ret = SetSecurityDescriptorSacl(sd, TRUE, pAcl, FALSE);
					if (!ret)
					{
						LocalFree(pAcl);
						goto cleanup;
					}
					break;
				}
				default:
				{
					SetLastError(ERROR_INVALID_PARAMETER);
					ret = FALSE;
					goto cleanup;
				}
			}
		}

		relativeSdSize = 0;
		while (TRUE)
		{
			if (relativeSd)
				LocalFree(relativeSd);
			relativeSd = LocalAlloc(0, relativeSdSize);
			if (!relativeSd)
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				goto cleanup;
			}
			ret = MakeSelfRelativeSD(sd, relativeSd, &relativeSdSize);
			if (ret || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
				break;
		}
		if (SecurityDescriptorSize)
			*SecurityDescriptorSize = relativeSdSize;
		*SecurityDescriptor = relativeSd;

cleanup:
		if (GetSecurityDescriptorOwner(sd, &pSid, &dummy))
			LocalFree(pSid);
		if (GetSecurityDescriptorGroup(sd, &pSid, &dummy))
			LocalFree(pSid);
		if (GetSecurityDescriptorDacl(sd, &present, &pAcl, &dummy) && present)
			LocalFree(pAcl);
		if (GetSecurityDescriptorSacl(sd, &present, &pAcl, &dummy) && present)
			LocalFree(pAcl);
		LocalFree(sd);
		return ret;
	}
	return FALSE;
}

/* Winehq cvs 20050916 */
/******************************************************************************
 * ConvertStringSecurityDescriptorToSecurityDescriptorA [ADVAPI32.@]
 * @implemented
 */
BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorA(
        LPCSTR StringSecurityDescriptor,
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

/*
 * @implemented
 */
BOOL STDCALL
EqualPrefixSid(PSID pSid1,
               PSID pSid2)
{
    return RtlEqualPrefixSid (pSid1, pSid2);
}


/*
 * @implemented
 */
BOOL STDCALL
EqualSid(PSID pSid1,
         PSID pSid2)
{
    return RtlEqualSid (pSid1, pSid2);
}


/*
 * @implemented
 *
 * RETURNS
 *  Docs says this function does NOT return a value
 *  even thou it's defined to return a PVOID...
 */
PVOID STDCALL
FreeSid(PSID pSid)
{
    return RtlFreeSid(pSid);
}


/*
 * @implemented
 */
DWORD STDCALL
GetLengthSid(PSID pSid)
{
    return (DWORD)RtlLengthSid(pSid);
}


/*
 * @implemented
 */
PSID_IDENTIFIER_AUTHORITY STDCALL
GetSidIdentifierAuthority(PSID pSid)
{
    return RtlIdentifierAuthoritySid(pSid);
}


/*
 * @implemented
 */
DWORD STDCALL
GetSidLengthRequired(UCHAR nSubAuthorityCount)
{
    return (DWORD)RtlLengthRequiredSid(nSubAuthorityCount);
}


/*
 * @implemented
 */
PDWORD STDCALL
GetSidSubAuthority(PSID pSid,
                   DWORD nSubAuthority)
{
    return (PDWORD)RtlSubAuthoritySid(pSid, nSubAuthority);
}


/*
 * @implemented
 */
PUCHAR STDCALL
GetSidSubAuthorityCount(PSID pSid)
{
    return RtlSubAuthorityCountSid(pSid);
}


/*
 * @implemented
 */
BOOL STDCALL
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
BOOL STDCALL
IsValidSid(PSID pSid)
{
    return (BOOL)RtlValidSid(pSid);
}


/*
 * @implemented
 */
BOOL STDCALL
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
BOOL STDCALL
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
BOOL STDCALL
EqualDomainSid(IN PSID pSid1,
               IN PSID pSid2,
               OUT BOOL* pfEqual)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetWindowsAccountDomainSid(IN PSID pSid,
                           OUT PSID ppDomainSid,
                           IN OUT DWORD* cbSid)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
CreateWellKnownSid(IN WELL_KNOWN_SID_TYPE WellKnownSidType,
                   IN PSID DomainSid  OPTIONAL,
                   OUT PSID pSid,
                   IN OUT DWORD* cbSid)
{
    int i;
    TRACE("(%d, %s, %p, %p)\n", WellKnownSidType, debugstr_sid(DomainSid), pSid, cbSid);

    if (DomainSid != NULL)
    {
        FIXME("Only local computer supported!\n");
        SetLastError(ERROR_INVALID_PARAMETER);	/* FIXME */
        return FALSE;
    }

    if (cbSid == NULL || pSid == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < sizeof(WellKnownSids)/sizeof(WellKnownSids[0]); i++)
    {
        if (WellKnownSids[i].Type == WellKnownSidType)
        {
            DWORD length = GetSidLengthRequired(WellKnownSids[i].Sid.SubAuthorityCount);

            if (*cbSid < length)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }

            CopyMemory(pSid, &WellKnownSids[i].Sid.Revision, length);
            *cbSid = length;
            return TRUE;
        }
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
IsWellKnownSid(IN PSID pSid,
               IN WELL_KNOWN_SID_TYPE WellKnownSidType)
{
    int i;
    TRACE("(%s, %d)\n", debugstr_sid(pSid), WellKnownSidType);

    for (i = 0; i < sizeof(WellKnownSids) / sizeof(WellKnownSids[0]); i++)
    {
        if (WellKnownSids[i].Type == WellKnownSidType)
        {
            if (EqualSid(pSid, (PSID)((ULONG_PTR)&WellKnownSids[i].Sid.Revision)))
                return TRUE;
        }
    }

    return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
ConvertStringSidToSidA(IN LPCSTR StringSid,
                       OUT PSID* sid)
{
    BOOL bRetVal = FALSE;

    if (!StringSid || !sid)
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        UINT len = MultiByteToWideChar(CP_ACP, 0, StringSid, -1, NULL, 0);
        LPWSTR wStringSid = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, StringSid, - 1, wStringSid, len);
        bRetVal = ConvertStringSidToSidW(wStringSid, sid);
        HeapFree(GetProcessHeap(), 0, wStringSid);
    }
    return bRetVal;
}


/******************************************************************************
 * ComputeStringSidSize
 */
static DWORD
ComputeStringSidSize(LPCWSTR StringSid)
{
    DWORD size = sizeof(SID);

    if (StringSid[0] == 'S' && StringSid[1] == '-') /* S-R-I-S-S */
    {
        int ctok = 0;
        while (*StringSid)
        {
            if (*StringSid == '-')
                ctok++;
            StringSid++;
        }

        if (ctok > 3)
            size += (ctok - 3) * sizeof(DWORD);
    }
    else /* String constant format  - Only available in winxp and above */
    {
        int i;

        for (i = 0; i < sizeof(WellKnownSids)/sizeof(WellKnownSids[0]); i++)
            if (!strncmpW(WellKnownSids[i].wstr, StringSid, 2))
                size += (WellKnownSids[i].Sid.SubAuthorityCount - 1) * sizeof(DWORD);
    }

    return size;
}

static const RECORD SidTable[] =
{
	{ SDDL_ACCOUNT_OPERATORS, WinBuiltinAccountOperatorsSid },
	{ SDDL_ALIAS_PREW2KCOMPACC, WinBuiltinPreWindows2000CompatibleAccessSid },
	{ SDDL_ANONYMOUS, WinAnonymousSid },
	{ SDDL_AUTHENTICATED_USERS, WinAuthenticatedUserSid },
	{ SDDL_BUILTIN_ADMINISTRATORS, WinBuiltinAdministratorsSid },
	{ SDDL_BUILTIN_GUESTS, WinBuiltinGuestsSid },
	{ SDDL_BACKUP_OPERATORS, WinBuiltinBackupOperatorsSid },
	{ SDDL_BUILTIN_USERS, WinBuiltinUsersSid },
	{ SDDL_CERT_SERV_ADMINISTRATORS, WinAccountCertAdminsSid /* FIXME: DOMAIN_GROUP_RID_CERT_ADMINS */ },
	{ SDDL_CREATOR_GROUP, WinCreatorGroupSid },
	{ SDDL_CREATOR_OWNER, WinCreatorOwnerSid },
	{ SDDL_DOMAIN_ADMINISTRATORS, WinAccountDomainAdminsSid /* FIXME: DOMAIN_GROUP_RID_ADMINS */ },
	{ SDDL_DOMAIN_COMPUTERS, WinAccountComputersSid /* FIXME: DOMAIN_GROUP_RID_COMPUTERS */ },
	{ SDDL_DOMAIN_DOMAIN_CONTROLLERS, WinAccountControllersSid /* FIXME: DOMAIN_GROUP_RID_CONTROLLERS */ },
	{ SDDL_DOMAIN_GUESTS, WinAccountDomainGuestsSid /* FIXME: DOMAIN_GROUP_RID_GUESTS */ },
	{ SDDL_DOMAIN_USERS, WinAccountDomainUsersSid /* FIXME: DOMAIN_GROUP_RID_USERS */ },
	{ SDDL_ENTERPRISE_ADMINS, WinAccountEnterpriseAdminsSid /* FIXME: DOMAIN_GROUP_RID_ENTERPRISE_ADMINS */ },
	{ SDDL_ENTERPRISE_DOMAIN_CONTROLLERS, WinLogonIdsSid /* FIXME: SECURITY_SERVER_LOGON_RID */ },
	{ SDDL_EVERYONE, WinWorldSid },
	{ SDDL_GROUP_POLICY_ADMINS, WinAccountPolicyAdminsSid /* FIXME: DOMAIN_GROUP_RID_POLICY_ADMINS */ },
	{ SDDL_INTERACTIVE, WinInteractiveSid },
	{ SDDL_LOCAL_ADMIN, WinAccountAdministratorSid /* FIXME: DOMAIN_USER_RID_ADMIN */ },
	{ SDDL_LOCAL_GUEST, WinAccountGuestSid /* FIXME: DOMAIN_USER_RID_GUEST */ },
	{ SDDL_LOCAL_SERVICE, WinLocalServiceSid },
	{ SDDL_LOCAL_SYSTEM, WinLocalSystemSid },
	{ SDDL_NETWORK, WinNetworkSid },
	{ SDDL_NETWORK_CONFIGURATION_OPS, WinBuiltinNetworkConfigurationOperatorsSid },
	{ SDDL_NETWORK_SERVICE, WinNetworkServiceSid },
	{ SDDL_PRINTER_OPERATORS, WinBuiltinPrintOperatorsSid },
	{ SDDL_PERSONAL_SELF, WinSelfSid },
	{ SDDL_POWER_USERS, WinBuiltinPowerUsersSid },
	{ SDDL_RAS_SERVERS, WinAccountRasAndIasServersSid /* FIXME: DOMAIN_ALIAS_RID_RAS_SERVERS */ },
	{ SDDL_REMOTE_DESKTOP, WinBuiltinRemoteDesktopUsersSid },
	{ SDDL_REPLICATOR, WinBuiltinReplicatorSid },
	{ SDDL_RESTRICTED_CODE, WinRestrictedCodeSid },
	{ SDDL_SCHEMA_ADMINISTRATORS, WinAccountSchemaAdminsSid /* FIXME: DOMAIN_GROUP_RID_SCHEMA_ADMINS */ },
	{ SDDL_SERVER_OPERATORS, WinBuiltinSystemOperatorsSid },
	{ SDDL_SERVICE, WinServiceSid },
	{ NULL, 0 },
};

/*
 * @implemented
 */
BOOL WINAPI
ConvertStringSidToSidW(IN LPCWSTR StringSid,
                       OUT PSID* sid)
{
    DWORD size;
    DWORD i, cBytes, identAuth, csubauth;
    BOOL ret;
    SID* pisid;

    TRACE("%s %p\n", debugstr_w(StringSid), sid);

	if (!StringSid)
	{
		SetLastError(ERROR_INVALID_SID);
		return FALSE;
	}
	for (i = 0; i < sizeof(SidTable) / sizeof(SidTable[0]) - 1; i++)
	{
		if (wcscmp(StringSid, SidTable[i].key) == 0)
		{
			WELL_KNOWN_SID_TYPE knownSid = (WELL_KNOWN_SID_TYPE)SidTable[i].value;
			size = SECURITY_MAX_SID_SIZE;
			*sid = LocalAlloc(0, size);
			if (!*sid)
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return FALSE;
			}
			ret = CreateWellKnownSid(
				knownSid,
				NULL,
				*sid,
				&size);
			if (!ret)
			{
				SetLastError(ERROR_INVALID_SID);
				LocalFree(*sid);
			}
			return ret;
		}
	}

	/* That's probably a string S-R-I-S-S... */
	if (StringSid[0] != 'S' || StringSid[1] != '-')
	{
		SetLastError(ERROR_INVALID_SID);
		return FALSE;
	}

    cBytes = ComputeStringSidSize(StringSid);
    pisid = (SID*)LocalAlloc( 0, cBytes );
    if (!pisid)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    i = 0;
    ret = FALSE;
    csubauth = ((cBytes - sizeof(SID)) / sizeof(DWORD)) + 1;

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
        while (*StringSid && *StringSid != '-')
            StringSid++;
        if (*StringSid == '-')
            StringSid++;

        pisid->SubAuthority[i++] = atoiW(StringSid);
    }

    if (i != pisid->SubAuthorityCount)
        goto lend; /* ERROR_INVALID_SID */

    *sid = pisid;
    ret = TRUE;

lend:
    if (!ret)
        SetLastError(ERROR_INVALID_SID);

    TRACE("returning %s\n", ret ? "TRUE" : "FALSE");
    return ret;
}


/* EOF */
