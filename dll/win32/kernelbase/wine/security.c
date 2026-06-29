/*
 * Copyright 1999, 2000 Juergen Schmied <juergen.schmied@debitel.net>
 * Copyright 2003 CodeWeavers Inc. (Ulrich Czekalla)
 * Copyright 2006 Robert Reif
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddk.h"

#include "kernelbase.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(security);


/******************************************************************************
 * SID functions
 ******************************************************************************/

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
    WELL_KNOWN_SID_TYPE Type;
    MAX_SID Sid;
} WELLKNOWNSID;

static const WELLKNOWNSID WellKnownSids[] =
{
    { WinNullSid, { SID_REVISION, 1, { SECURITY_NULL_SID_AUTHORITY }, { SECURITY_NULL_RID } } },
    { WinWorldSid, { SID_REVISION, 1, { SECURITY_WORLD_SID_AUTHORITY }, { SECURITY_WORLD_RID } } },
    { WinLocalSid, { SID_REVISION, 1, { SECURITY_LOCAL_SID_AUTHORITY }, { SECURITY_LOCAL_RID } } },
    { WinCreatorOwnerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RID } } },
    { WinCreatorGroupSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_RID } } },
    { WinCreatorOwnerRightsSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_RIGHTS_RID } } },
    { WinCreatorOwnerServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_OWNER_SERVER_RID } } },
    { WinCreatorGroupServerSid, { SID_REVISION, 1, { SECURITY_CREATOR_SID_AUTHORITY }, { SECURITY_CREATOR_GROUP_SERVER_RID } } },
    { WinNtAuthoritySid, { SID_REVISION, 0, { SECURITY_NT_AUTHORITY }, { SECURITY_NULL_RID } } },
    { WinDialupSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_DIALUP_RID } } },
    { WinNetworkSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_RID } } },
    { WinBatchSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BATCH_RID } } },
    { WinInteractiveSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_INTERACTIVE_RID } } },
    { WinServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_SERVICE_RID } } },
    { WinAnonymousSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ANONYMOUS_LOGON_RID } } },
    { WinProxySid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PROXY_RID } } },
    { WinEnterpriseControllersSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_ENTERPRISE_CONTROLLERS_RID } } },
    { WinSelfSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_PRINCIPAL_SELF_RID } } },
    { WinAuthenticatedUserSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_AUTHENTICATED_USER_RID } } },
    { WinRestrictedCodeSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_RESTRICTED_CODE_RID } } },
    { WinTerminalServerSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_TERMINAL_SERVER_RID } } },
    { WinRemoteLogonIdSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_REMOTE_LOGON_RID } } },
    { WinLogonIdsSid, { SID_REVISION, SECURITY_LOGON_IDS_RID_COUNT, { SECURITY_NT_AUTHORITY }, { SECURITY_LOGON_IDS_RID } } },
    { WinLocalSystemSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } } },
    { WinLocalServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SERVICE_RID } } },
    { WinNetworkServiceSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_NETWORK_SERVICE_RID } } },
    { WinBuiltinDomainSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID } } },
    { WinBuiltinAdministratorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS } } },
    { WinBuiltinUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS } } },
    { WinBuiltinGuestsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS } } },
    { WinBuiltinPowerUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS } } },
    { WinBuiltinAccountOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ACCOUNT_OPS } } },
    { WinBuiltinSystemOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS } } },
    { WinBuiltinPrintOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PRINT_OPS } } },
    { WinBuiltinBackupOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_BACKUP_OPS } } },
    { WinBuiltinReplicatorSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REPLICATOR } } },
    { WinBuiltinPreWindows2000CompatibleAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_PREW2KCOMPACCESS } } },
    { WinBuiltinRemoteDesktopUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS } } },
    { WinBuiltinNetworkConfigurationOperatorsSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS } } },
    { WinNTLMAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_NTLM_RID } } },
    { WinDigestAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_DIGEST_RID } } },
    { WinSChannelAuthenticationSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_PACKAGE_BASE_RID, SECURITY_PACKAGE_SCHANNEL_RID } } },
    { WinThisOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_THIS_ORGANIZATION_RID } } },
    { WinOtherOrganizationSid, { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_OTHER_ORGANIZATION_RID } } },
    { WinBuiltinIncomingForestTrustBuildersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS  } } },
    { WinBuiltinPerfMonitoringUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_MONITORING_USERS } } },
    { WinBuiltinPerfLoggingUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_LOGGING_USERS } } },
    { WinBuiltinAuthorizationAccessSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS } } },
    { WinBuiltinTerminalServerLicenseServersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS } } },
    { WinBuiltinDCOMUsersSid, { SID_REVISION, 2, { SECURITY_NT_AUTHORITY }, { SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_DCOM_USERS } } },
    { WinLowLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_LOW_RID} } },
    { WinMediumLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_MEDIUM_RID } } },
    { WinHighLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_HIGH_RID } } },
    { WinSystemLabelSid, { SID_REVISION, 1, { SECURITY_MANDATORY_LABEL_AUTHORITY}, { SECURITY_MANDATORY_SYSTEM_RID } } },
    { WinBuiltinAnyPackageSid, { SID_REVISION, 2, { SECURITY_APP_PACKAGE_AUTHORITY }, { SECURITY_APP_PACKAGE_BASE_RID, SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE } } },
};

/* these SIDs must be constructed as relative to some domain - only the RID is well-known */
typedef struct WELLKNOWNRID
{
    WELL_KNOWN_SID_TYPE Type;
    DWORD Rid;
} WELLKNOWNRID;

static const WELLKNOWNRID WellKnownRids[] =
{
    { WinAccountAdministratorSid,    DOMAIN_USER_RID_ADMIN },
    { WinAccountGuestSid,            DOMAIN_USER_RID_GUEST },
    { WinAccountKrbtgtSid,           DOMAIN_USER_RID_KRBTGT },
    { WinAccountDomainAdminsSid,     DOMAIN_GROUP_RID_ADMINS },
    { WinAccountDomainUsersSid,      DOMAIN_GROUP_RID_USERS },
    { WinAccountDomainGuestsSid,     DOMAIN_GROUP_RID_GUESTS },
    { WinAccountComputersSid,        DOMAIN_GROUP_RID_COMPUTERS },
    { WinAccountControllersSid,      DOMAIN_GROUP_RID_CONTROLLERS },
    { WinAccountCertAdminsSid,       DOMAIN_GROUP_RID_CERT_ADMINS },
    { WinAccountSchemaAdminsSid,     DOMAIN_GROUP_RID_SCHEMA_ADMINS },
    { WinAccountEnterpriseAdminsSid, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS },
    { WinAccountPolicyAdminsSid,     DOMAIN_GROUP_RID_POLICY_ADMINS },
    { WinAccountRasAndIasServersSid, DOMAIN_ALIAS_RID_RAS_SERVERS },
};

static NTSTATUS open_file( LPCWSTR name, DWORD access, HANDLE *file )
{
    UNICODE_STRING file_nameW;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if ((status = RtlDosPathNameToNtPathName_U_WithStatus( name, &file_nameW, NULL, NULL ))) return status;
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &file_nameW;
    attr.SecurityDescriptor = NULL;
    status = NtCreateFile( file, access|SYNCHRONIZE, &attr, &io, NULL, FILE_FLAG_BACKUP_SEMANTICS,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_OPEN,
                           FILE_OPEN_FOR_BACKUP_INTENT, NULL, 0 );
    RtlFreeUnicodeString( &file_nameW );
    return status;
}

static const char *debugstr_sid( PSID sid )
{
    int auth;
    SID * psid = sid;

    if (psid == NULL) return "(null)";

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

/******************************************************************************
 * AllocateAndInitializeSid   (kernelbase.@)
 */
BOOL WINAPI AllocateAndInitializeSid( PSID_IDENTIFIER_AUTHORITY auth, BYTE count,
                                      DWORD auth0, DWORD auth1, DWORD auth2, DWORD auth3,
                                      DWORD auth4, DWORD auth5, DWORD auth6, DWORD auth7, PSID *sid )
{
    return set_ntstatus( RtlAllocateAndInitializeSid( auth, count, auth0, auth1, auth2, auth3,
                                                      auth4, auth5, auth6, auth7, sid ));
}

/***********************************************************************
 * AllocateLocallyUniqueId   (kernelbase.@)
 */
BOOL WINAPI AllocateLocallyUniqueId( PLUID luid )
{
    return set_ntstatus( NtAllocateLocallyUniqueId( luid ));
}

/******************************************************************************
 * CopySid   (kernelbase.@)
 */
BOOL WINAPI CopySid( DWORD len, PSID dest, PSID source )
{
    return RtlCopySid( len, dest, source );
}

/******************************************************************************
 * EqualPrefixSid   (kernelbase.@)
 */
BOOL WINAPI EqualPrefixSid( PSID sid1, PSID sid2 )
{
    return RtlEqualPrefixSid( sid1, sid2 );
}

/******************************************************************************
 * EqualSid   (kernelbase.@)
 */
BOOL WINAPI EqualSid( PSID sid1, PSID sid2 )
{
    BOOL ret = RtlEqualSid( sid1, sid2 );
    SetLastError(ERROR_SUCCESS);
    return ret;
}

/******************************************************************************
 * EqualDomainSid   (kernelbase.@)
 */
BOOL WINAPI EqualDomainSid( PSID sid1, PSID sid2, BOOL *equal )
{
    MAX_SID builtin_sid, domain_sid1, domain_sid2;
    DWORD size;

    TRACE( "(%p,%p,%p)\n", sid1, sid2, equal );

    if (!IsValidSid( sid1 ) || !IsValidSid( sid2 ))
    {
        SetLastError( ERROR_INVALID_SID );
        return FALSE;
    }

    if (!equal)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    size = sizeof(domain_sid1);
    if (GetWindowsAccountDomainSid( sid1, &domain_sid1, &size ))
    {
        size = sizeof(domain_sid2);
        if (GetWindowsAccountDomainSid( sid2, &domain_sid2, &size ))
        {
            *equal = EqualSid( &domain_sid1, &domain_sid2 );
            SetLastError( 0 );
            return TRUE;
        }
    }

    size = sizeof(builtin_sid);
    if (!CreateWellKnownSid( WinBuiltinDomainSid, NULL, &builtin_sid, &size ))
        return FALSE;

    if (!memcmp(GetSidIdentifierAuthority( sid1 )->Value, builtin_sid.IdentifierAuthority.Value, sizeof(builtin_sid.IdentifierAuthority.Value)) &&
        !memcmp(GetSidIdentifierAuthority( sid2 )->Value, builtin_sid.IdentifierAuthority.Value, sizeof(builtin_sid.IdentifierAuthority.Value)))
    {
        if (*GetSidSubAuthorityCount( sid1 ) != 0 && *GetSidSubAuthorityCount( sid2 ) != 0 &&
            (*GetSidSubAuthority( sid1, 0 ) == SECURITY_BUILTIN_DOMAIN_RID ||
             *GetSidSubAuthority( sid2, 0 ) == SECURITY_BUILTIN_DOMAIN_RID))
        {
            *equal = EqualSid( sid1, sid2 );
            SetLastError( 0 );
            return TRUE;
        }
    }

    SetLastError( ERROR_NON_DOMAIN_SID );
    return FALSE;
}

/******************************************************************************
 * FreeSid   (kernelbase.@)
 */
void * WINAPI FreeSid( PSID pSid )
{
    RtlFreeSid(pSid);
    return NULL; /* is documented like this */
}

/******************************************************************************
 * GetLengthSid   (kernelbase.@)
 */
DWORD WINAPI GetLengthSid( PSID sid )
{
    return RtlLengthSid( sid );
}

/******************************************************************************
 * GetSidIdentifierAuthority   (kernelbase.@)
 */
PSID_IDENTIFIER_AUTHORITY WINAPI GetSidIdentifierAuthority( PSID sid )
{
    SetLastError(ERROR_SUCCESS);
    return RtlIdentifierAuthoritySid( sid );
}

/******************************************************************************
 * GetSidLengthRequired   (kernelbase.@)
 */
DWORD WINAPI GetSidLengthRequired( BYTE count )
{
    return RtlLengthRequiredSid( count );
}

/******************************************************************************
 * GetSidSubAuthority   (kernelbase.@)
 */
PDWORD WINAPI GetSidSubAuthority( PSID sid, DWORD auth )
{
    SetLastError(ERROR_SUCCESS);
    return RtlSubAuthoritySid( sid, auth );
}

/******************************************************************************
 * GetSidSubAuthorityCount   (kernelbase.@)
 */
PUCHAR WINAPI GetSidSubAuthorityCount( PSID sid )
{
    SetLastError(ERROR_SUCCESS);
    return RtlSubAuthorityCountSid( sid );
}

/******************************************************************************
 * GetWindowsAccountDomainSid    (kernelbase.@)
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
        SetLastError( domain_sid ? ERROR_INSUFFICIENT_BUFFER : ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    InitializeSid( domain_sid, &domain_ident, 4 );
    for (i = 0; i < 4; i++)
        *GetSidSubAuthority( domain_sid, i ) = *GetSidSubAuthority( sid, i );

    *size = required_size;
    return TRUE;
}

/******************************************************************************
 * InitializeSid   (kernelbase.@)
 */
BOOL WINAPI InitializeSid ( PSID sid, PSID_IDENTIFIER_AUTHORITY auth, BYTE count )
{
    return set_ntstatus(RtlInitializeSid( sid, auth, count ));
}

/******************************************************************************
 * IsValidSid   (kernelbase.@)
 */
BOOL WINAPI IsValidSid( PSID sid )
{
    return RtlValidSid( sid );
}

/******************************************************************************
 * CreateWellKnownSid   (kernelbase.@)
 */
BOOL WINAPI CreateWellKnownSid( WELL_KNOWN_SID_TYPE type, PSID domain, PSID sid, DWORD *size )
{
    unsigned int i;

    TRACE("(%d, %s, %p, %p)\n", type, debugstr_sid(domain), sid, size);

    if (size == NULL || (domain && !IsValidSid(domain)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(WellKnownSids); i++)
    {
        if (WellKnownSids[i].Type == type)
        {
            DWORD length = GetSidLengthRequired(WellKnownSids[i].Sid.SubAuthorityCount);

            if (*size < length)
            {
                *size = length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!sid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(sid, &WellKnownSids[i].Sid.Revision, length);
            *size = length;
            return TRUE;
        }
    }

    if (domain == NULL || *GetSidSubAuthorityCount(domain) == SID_MAX_SUB_AUTHORITIES)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(WellKnownRids); i++)
    {
        if (WellKnownRids[i].Type == type)
        {
            UCHAR domain_subauth = *GetSidSubAuthorityCount(domain);
            DWORD domain_sid_length = GetSidLengthRequired(domain_subauth);
            DWORD output_sid_length = GetSidLengthRequired(domain_subauth + 1);

            if (*size < output_sid_length)
            {
                *size = output_sid_length;
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
            if (!sid)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CopyMemory(sid, domain, domain_sid_length);
            (*GetSidSubAuthorityCount(sid))++;
            (*GetSidSubAuthority(sid, domain_subauth)) = WellKnownRids[i].Rid;
            *size = output_sid_length;
            return TRUE;
        }
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/******************************************************************************
 * IsWellKnownSid   (kernelbase.@)
 */
BOOL WINAPI IsWellKnownSid( PSID sid, WELL_KNOWN_SID_TYPE type )
{
    unsigned int i;

    TRACE("(%s, %d)\n", debugstr_sid(sid), type);

    for (i = 0; i < ARRAY_SIZE(WellKnownSids); i++)
        if (WellKnownSids[i].Type == type)
            if (EqualSid(sid, (PSID)&WellKnownSids[i].Sid.Revision))
                return TRUE;

    return FALSE;
}


/******************************************************************************
 * Token functions
 ******************************************************************************/


/******************************************************************************
 * AdjustTokenGroups    (kernelbase.@)
 */
BOOL WINAPI AdjustTokenGroups( HANDLE token, BOOL reset, PTOKEN_GROUPS new,
                               DWORD len, PTOKEN_GROUPS prev, PDWORD ret_len )
{
    return set_ntstatus( NtAdjustGroupsToken( token, reset, new, len, prev, ret_len ));
}

/******************************************************************************
 * AdjustTokenPrivileges    (kernelbase.@)
 */
BOOL WINAPI AdjustTokenPrivileges( HANDLE token, BOOL disable, PTOKEN_PRIVILEGES new, DWORD len,
                                   PTOKEN_PRIVILEGES prev, PDWORD ret_len )
{
    NTSTATUS status;

    TRACE("(%p %d %p %ld %p %p)\n", token, disable, new, len, prev, ret_len );

    status = NtAdjustPrivilegesToken( token, disable, new, len, prev, ret_len );
    SetLastError( RtlNtStatusToDosError( status ));
    return (status == STATUS_SUCCESS) || (status == STATUS_NOT_ALL_ASSIGNED);
}

/******************************************************************************
 * CheckTokenMembership    (kernelbase.@)
 */
BOOL WINAPI CheckTokenMembership( HANDLE token, PSID sid_to_check, PBOOL is_member )
{
    PTOKEN_GROUPS token_groups = NULL;
    HANDLE thread_token = NULL;
    DWORD size, i;
    BOOL ret;

    TRACE("(%p %s %p)\n", token, debugstr_sid(sid_to_check), is_member);

    *is_member = FALSE;

    if (!token)
    {
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &thread_token))
        {
            HANDLE process_token;
            ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &process_token);
            if (!ret)
                goto exit;
            ret = DuplicateTokenEx(process_token, TOKEN_QUERY, NULL, SecurityImpersonation,
                                   TokenImpersonation, &thread_token);
            CloseHandle(process_token);
            if (!ret)
                goto exit;
        }
        token = thread_token;
    }
    else
    {
        TOKEN_TYPE type;

        ret = GetTokenInformation(token, TokenType, &type, sizeof(TOKEN_TYPE), &size);
        if (!ret) goto exit;

        if (type == TokenPrimary)
        {
            SetLastError(ERROR_NO_IMPERSONATION_TOKEN);
            return FALSE;
        }
    }

    ret = GetTokenInformation(token, TokenGroups, NULL, 0, &size);
    if (!ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        goto exit;

    token_groups = heap_alloc(size);
    if (!token_groups)
    {
        ret = FALSE;
        goto exit;
    }

    ret = GetTokenInformation(token, TokenGroups, token_groups, size, &size);
    if (!ret)
        goto exit;

    for (i = 0; i < token_groups->GroupCount; i++)
    {
        TRACE("Groups[%ld]: {0x%lx, %s}\n", i,
            token_groups->Groups[i].Attributes,
            debugstr_sid(token_groups->Groups[i].Sid));
        if ((token_groups->Groups[i].Attributes & SE_GROUP_ENABLED) &&
            EqualSid(sid_to_check, token_groups->Groups[i].Sid))
        {
            *is_member = TRUE;
            TRACE("sid enabled and found in token\n");
            break;
        }
    }

exit:
    heap_free(token_groups);
    if (thread_token != NULL) CloseHandle(thread_token);
    return ret;
}

/*************************************************************************
 * CreateRestrictedToken    (kernelbase.@)
 */
BOOL WINAPI CreateRestrictedToken( HANDLE token, DWORD flags,
                                   DWORD disable_sid_count, SID_AND_ATTRIBUTES *disable_sids,
                                   DWORD delete_priv_count, LUID_AND_ATTRIBUTES *delete_privs,
                                   DWORD restrict_sid_count, SID_AND_ATTRIBUTES *restrict_sids, HANDLE *ret )
{
    TOKEN_PRIVILEGES *nt_privs = NULL;
    TOKEN_GROUPS *nt_disable_sids = NULL, *nt_restrict_sids = NULL;
    NTSTATUS status = STATUS_NO_MEMORY;

    TRACE("token %p, flags %#lx, disable_sids %lu %p, delete_privs %lu %p, restrict_sids %lu %p, ret %p\n",
            token, flags, disable_sid_count, disable_sids, delete_priv_count, delete_privs,
            restrict_sid_count, restrict_sids, ret);

    if (disable_sid_count)
    {
        if (!(nt_disable_sids = heap_alloc( offsetof( TOKEN_GROUPS, Groups[disable_sid_count] ) ))) goto out;
        nt_disable_sids->GroupCount = disable_sid_count;
        memcpy( nt_disable_sids->Groups, disable_sids, disable_sid_count * sizeof(*disable_sids) );
    }

    if (delete_priv_count)
    {
        if (!(nt_privs = heap_alloc( offsetof( TOKEN_PRIVILEGES, Privileges[delete_priv_count] ) ))) goto out;
        nt_privs->PrivilegeCount = delete_priv_count;
        memcpy( nt_privs->Privileges, delete_privs, delete_priv_count * sizeof(*delete_privs) );
    }

    if (restrict_sid_count)
    {
        if (!(nt_restrict_sids = heap_alloc( offsetof( TOKEN_GROUPS, Groups[restrict_sid_count] ) ))) goto out;
        nt_restrict_sids->GroupCount = restrict_sid_count;
        memcpy( nt_restrict_sids->Groups, restrict_sids, restrict_sid_count * sizeof(*restrict_sids) );
    }

    status = NtFilterToken(token, flags, nt_disable_sids, nt_privs, nt_restrict_sids, ret);

out:
    heap_free(nt_disable_sids);
    heap_free(nt_privs);
    heap_free(nt_restrict_sids);
    return set_ntstatus( status );
}

/******************************************************************************
 * DuplicateToken    (kernelbase.@)
 */
BOOL WINAPI DuplicateToken( HANDLE token, SECURITY_IMPERSONATION_LEVEL level, PHANDLE ret )
{
    return DuplicateTokenEx( token, TOKEN_IMPERSONATE|TOKEN_QUERY, NULL, level, TokenImpersonation, ret );
}

/******************************************************************************
 * DuplicateTokenEx    (kernelbase.@)
 */
BOOL WINAPI DuplicateTokenEx( HANDLE token, DWORD access, LPSECURITY_ATTRIBUTES sa,
                              SECURITY_IMPERSONATION_LEVEL level, TOKEN_TYPE type, PHANDLE ret )
{
    SECURITY_QUALITY_OF_SERVICE qos;
    OBJECT_ATTRIBUTES attr;

    TRACE("%p 0x%08lx 0x%08x 0x%08x %p\n", token, access, level, type, ret );

    qos.Length = sizeof(qos);
    qos.ImpersonationLevel = level;
    qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    qos.EffectiveOnly = FALSE;
    InitializeObjectAttributes( &attr, NULL, (sa && sa->bInheritHandle) ? OBJ_INHERIT : 0,
                                NULL, sa ? sa->lpSecurityDescriptor : NULL );
    attr.SecurityQualityOfService = &qos;
    return set_ntstatus( NtDuplicateToken( token, access, &attr, FALSE, type, ret ));
}

/******************************************************************************
 * GetTokenInformation    (kernelbase.@)
 */
BOOL WINAPI GetTokenInformation( HANDLE token, TOKEN_INFORMATION_CLASS class,
                                 LPVOID info, DWORD len, LPDWORD retlen )
{
    TRACE("(%p, %d [%s], %p, %ld, %p):\n",
          token, class,
          (class == TokenUser) ? "TokenUser" :
          (class == TokenGroups) ? "TokenGroups" :
          (class == TokenPrivileges) ? "TokenPrivileges" :
          (class == TokenOwner) ? "TokenOwner" :
          (class == TokenPrimaryGroup) ? "TokenPrimaryGroup" :
          (class == TokenDefaultDacl) ? "TokenDefaultDacl" :
          (class == TokenSource) ? "TokenSource" :
          (class == TokenType) ? "TokenType" :
          (class == TokenImpersonationLevel) ? "TokenImpersonationLevel" :
          (class == TokenStatistics) ? "TokenStatistics" :
          (class == TokenRestrictedSids) ? "TokenRestrictedSids" :
          (class == TokenSessionId) ? "TokenSessionId" :
          (class == TokenGroupsAndPrivileges) ? "TokenGroupsAndPrivileges" :
          (class == TokenSessionReference) ? "TokenSessionReference" :
          (class == TokenSandBoxInert) ? "TokenSandBoxInert" :
          (class == TokenElevation) ? "TokenElevation" :
          (class == TokenElevationType) ? "TokenElevationType" :
          (class == TokenLinkedToken) ? "TokenLinkedToken" :
          "Unknown",
          info, len, retlen);

    return set_ntstatus( NtQueryInformationToken( token, class, info, len, retlen ));
}

/******************************************************************************
 * ImpersonateAnonymousToken    (kernelbase.@)
 */
BOOL WINAPI ImpersonateAnonymousToken( HANDLE thread )
{
    TRACE("(%p)\n", thread);
    return set_ntstatus( NtImpersonateAnonymousToken( thread ) );
}

/******************************************************************************
 * ImpersonateLoggedOnUser    (kernelbase.@)
 */
BOOL WINAPI ImpersonateLoggedOnUser( HANDLE token )
{
    DWORD size;
    BOOL ret;
    HANDLE dup;
    TOKEN_TYPE type;
    static BOOL warn = TRUE;

    if (warn)
    {
        FIXME( "(%p)\n", token );
        warn = FALSE;
    }
    if (!GetTokenInformation( token, TokenType, &type, sizeof(type), &size )) return FALSE;

    if (type == TokenPrimary)
    {
        if (!DuplicateToken( token, SecurityImpersonation, &dup )) return FALSE;
        ret = SetThreadToken( NULL, dup );
        NtClose( dup );
    }
    else ret = SetThreadToken( NULL, token );

    return ret;
}

/******************************************************************************
 * ImpersonateNamedPipeClient    (kernelbase.@)
 */
BOOL WINAPI ImpersonateNamedPipeClient( HANDLE pipe )
{
    IO_STATUS_BLOCK io_block;

    return set_ntstatus( NtFsControlFile( pipe, NULL, NULL, NULL, &io_block,
                                          FSCTL_PIPE_IMPERSONATE, NULL, 0, NULL, 0 ));
}

/******************************************************************************
 * ImpersonateSelf    (kernelbase.@)
 */
BOOL WINAPI ImpersonateSelf( SECURITY_IMPERSONATION_LEVEL level )
{
    return set_ntstatus( RtlImpersonateSelf( level ) );
}

/******************************************************************************
 * IsTokenRestricted    (kernelbase.@)
 */
BOOL WINAPI IsTokenRestricted( HANDLE token )
{
    TOKEN_GROUPS *groups;
    DWORD size;
    NTSTATUS status;
    BOOL restricted;

    TRACE("(%p)\n", token);

    status = NtQueryInformationToken(token, TokenRestrictedSids, NULL, 0, &size);
    if (status != STATUS_BUFFER_TOO_SMALL) return set_ntstatus(status);

    groups = heap_alloc(size);
    if (!groups)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    status = NtQueryInformationToken(token, TokenRestrictedSids, groups, size, &size);
    if (status != STATUS_SUCCESS)
    {
        heap_free(groups);
        return set_ntstatus(status);
    }

    restricted = groups->GroupCount > 0;
    heap_free(groups);

    return restricted;
}

/******************************************************************************
 * OpenProcessToken    (kernelbase.@)
 */
BOOL WINAPI OpenProcessToken( HANDLE process, DWORD access, HANDLE *handle )
{
    return set_ntstatus( NtOpenProcessToken( process, access, handle ));
}

/******************************************************************************
 * OpenThreadToken    (kernelbase.@)
 */
BOOL WINAPI OpenThreadToken( HANDLE thread, DWORD access, BOOL self, HANDLE *handle )
{
    return set_ntstatus( NtOpenThreadToken( thread, access, self, handle ));
}

/******************************************************************************
 * PrivilegeCheck    (kernelbase.@)
 */
BOOL WINAPI PrivilegeCheck( HANDLE token, PPRIVILEGE_SET privs, LPBOOL result )
{
    BOOLEAN res;
    BOOL ret = set_ntstatus( NtPrivilegeCheck( token, privs, &res ));
    if (ret) *result = res;
    return ret;
}

/******************************************************************************
 * RevertToSelf    (kernelbase.@)
 */
BOOL WINAPI RevertToSelf(void)
{
    return SetThreadToken( NULL, 0 );
}

/*************************************************************************
 * SetThreadToken    (kernelbase.@)
 */
BOOL WINAPI SetThreadToken( PHANDLE thread, HANDLE token )
{
    return set_ntstatus( NtSetInformationThread( thread ? *thread : GetCurrentThread(),
                                                 ThreadImpersonationToken, &token, sizeof(token) ));
}

/******************************************************************************
 * SetTokenInformation    (kernelbase.@)
 */
BOOL WINAPI SetTokenInformation( HANDLE token, TOKEN_INFORMATION_CLASS class, LPVOID info, DWORD len )
{
    TRACE("(%p, %s, %p, %ld)\n",
          token,
          (class == TokenUser) ? "TokenUser" :
          (class == TokenGroups) ? "TokenGroups" :
          (class == TokenPrivileges) ? "TokenPrivileges" :
          (class == TokenOwner) ? "TokenOwner" :
          (class == TokenPrimaryGroup) ? "TokenPrimaryGroup" :
          (class == TokenDefaultDacl) ? "TokenDefaultDacl" :
          (class == TokenSource) ? "TokenSource" :
          (class == TokenType) ? "TokenType" :
          (class == TokenImpersonationLevel) ? "TokenImpersonationLevel" :
          (class == TokenStatistics) ? "TokenStatistics" :
          (class == TokenRestrictedSids) ? "TokenRestrictedSids" :
          (class == TokenSessionId) ? "TokenSessionId" :
          (class == TokenGroupsAndPrivileges) ? "TokenGroupsAndPrivileges" :
          (class == TokenSessionReference) ? "TokenSessionReference" :
          (class == TokenSandBoxInert) ? "TokenSandBoxInert" :
          "Unknown",
          info, len);

    return set_ntstatus( NtSetInformationToken( token, class, info, len ));
}


/******************************************************************************
 * Security descriptor functions
 ******************************************************************************/


/******************************************************************************
 * ConvertToAutoInheritPrivateObjectSecurity    (kernelbase.@)
 */
BOOL WINAPI ConvertToAutoInheritPrivateObjectSecurity( PSECURITY_DESCRIPTOR parent,
                                                       PSECURITY_DESCRIPTOR current,
                                                       PSECURITY_DESCRIPTOR *descr,
                                                       GUID *type, BOOL is_dir,
                                                       PGENERIC_MAPPING mapping )
{
    return set_ntstatus( RtlConvertToAutoInheritSecurityObject( parent, current, descr, type, is_dir, mapping ));
}

/******************************************************************************
 * CreateBoundaryDescriptorW    (kernelbase.@)
 */
HANDLE WINAPI CreateBoundaryDescriptorW( LPCWSTR name, ULONG flags )
{
    FIXME("%s %lu - stub\n", debugstr_w(name), flags);
    return NULL;
}

/******************************************************************************
 * CreatePrivateObjectSecurity    (kernelbase.@)
 */
BOOL WINAPI CreatePrivateObjectSecurity( PSECURITY_DESCRIPTOR parent, PSECURITY_DESCRIPTOR creator,
                                         PSECURITY_DESCRIPTOR *descr, BOOL is_container, HANDLE token,
                                         PGENERIC_MAPPING mapping )
{
    return set_ntstatus( RtlNewSecurityObject( parent, creator, descr, is_container, token, mapping ));
}

/******************************************************************************
 * CreatePrivateObjectSecurityEx    (kernelbase.@)
 */
BOOL WINAPI CreatePrivateObjectSecurityEx( PSECURITY_DESCRIPTOR parent, PSECURITY_DESCRIPTOR creator,
                                           PSECURITY_DESCRIPTOR *descr, GUID *type, BOOL is_container,
                                           ULONG flags, HANDLE token, PGENERIC_MAPPING mapping )
{
    return set_ntstatus( RtlNewSecurityObjectEx( parent, creator, descr, type, is_container, flags, token, mapping ));
}

/******************************************************************************
 * CreatePrivateObjectSecurityWithMultipleInheritance    (kernelbase.@)
 */
BOOL WINAPI CreatePrivateObjectSecurityWithMultipleInheritance( PSECURITY_DESCRIPTOR parent,
                                                                PSECURITY_DESCRIPTOR creator,
                                                                PSECURITY_DESCRIPTOR *descr,
                                                                GUID **types, ULONG count,
                                                                BOOL is_container, ULONG flags,
                                                                HANDLE token, PGENERIC_MAPPING mapping )
{
    return set_ntstatus( RtlNewSecurityObjectWithMultipleInheritance( parent, creator, descr, types, count,
            is_container, flags, token, mapping ));
}

/******************************************************************************
 * DestroyPrivateObjectSecurity    (kernelbase.@)
 */
BOOL WINAPI DestroyPrivateObjectSecurity( PSECURITY_DESCRIPTOR *descr )
{
    return set_ntstatus( RtlDeleteSecurityObject( descr ));
}

/******************************************************************************
 * GetFileSecurityW    (kernelbase.@)
 */
BOOL WINAPI GetFileSecurityW( LPCWSTR name, SECURITY_INFORMATION info,
                              PSECURITY_DESCRIPTOR descr, DWORD len, LPDWORD ret_len )
{
    HANDLE file;
    NTSTATUS status;
    DWORD access = 0;

    TRACE( "(%s,%ld,%p,%ld,%p)\n", debugstr_w(name), info, descr, len, ret_len );

    if (info & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION))
        access |= READ_CONTROL;
    if (info & SACL_SECURITY_INFORMATION)
        access |= ACCESS_SYSTEM_SECURITY;

    if (!(status = open_file( name, access, &file )))
    {
        status = NtQuerySecurityObject( file, info, descr, len, ret_len );
        NtClose( file );
    }
    return set_ntstatus( status );
}

/******************************************************************************
 * GetKernelObjectSecurity    (kernelbase.@)
 */
BOOL WINAPI GetKernelObjectSecurity( HANDLE handle, SECURITY_INFORMATION info,
                                     PSECURITY_DESCRIPTOR descr, DWORD len, LPDWORD ret_len )
{
    return set_ntstatus( NtQuerySecurityObject( handle, info, descr, len, ret_len ));
}

/******************************************************************************
 * GetPrivateObjectSecurity    (kernelbase.@)
 */
BOOL WINAPI GetPrivateObjectSecurity( PSECURITY_DESCRIPTOR obj_descr, SECURITY_INFORMATION info,
                                      PSECURITY_DESCRIPTOR ret_descr, DWORD len, PDWORD ret_len )
{
    SECURITY_DESCRIPTOR desc;
    BOOL defaulted, present;
    PACL pacl;
    PSID psid;

    TRACE("(%p,0x%08lx,%p,0x%08lx,%p)\n", obj_descr, info, ret_descr, len, ret_len );

    if (!InitializeSecurityDescriptor(&desc, SECURITY_DESCRIPTOR_REVISION)) return FALSE;

    if (info & OWNER_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorOwner(obj_descr, &psid, &defaulted)) return FALSE;
        SetSecurityDescriptorOwner(&desc, psid, defaulted);
    }
    if (info & GROUP_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorGroup(obj_descr, &psid, &defaulted)) return FALSE;
        SetSecurityDescriptorGroup(&desc, psid, defaulted);
    }
    if (info & DACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorDacl(obj_descr, &present, &pacl, &defaulted)) return FALSE;
        SetSecurityDescriptorDacl(&desc, present, pacl, defaulted);
    }
    if (info & SACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorSacl(obj_descr, &present, &pacl, &defaulted)) return FALSE;
        SetSecurityDescriptorSacl(&desc, present, pacl, defaulted);
    }

    *ret_len = len;
    return MakeSelfRelativeSD(&desc, ret_descr, ret_len);
}

/******************************************************************************
 * GetSecurityDescriptorControl    (kernelbase.@)
 */
BOOL WINAPI GetSecurityDescriptorControl( PSECURITY_DESCRIPTOR descr, PSECURITY_DESCRIPTOR_CONTROL control,
                                          LPDWORD revision)
{
    return set_ntstatus( RtlGetControlSecurityDescriptor( descr, control, revision ));
}

/******************************************************************************
 *  GetSecurityDescriptorDacl    (kernelbase.@)
 */
BOOL WINAPI GetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR descr, LPBOOL dacl_present, PACL *dacl,
                                       LPBOOL dacl_defaulted )
{
    BOOLEAN present, defaulted;
    BOOL ret = set_ntstatus( RtlGetDaclSecurityDescriptor( descr, &present, dacl, &defaulted ));
    *dacl_present = present;
    *dacl_defaulted = defaulted;
    return ret;
}

/******************************************************************************
 * GetSecurityDescriptorGroup    (kernelbase.@)
 */
BOOL WINAPI GetSecurityDescriptorGroup( PSECURITY_DESCRIPTOR descr, PSID *group, LPBOOL group_defaulted )
{
    BOOLEAN defaulted;
    BOOL ret = set_ntstatus( RtlGetGroupSecurityDescriptor( descr, group, &defaulted ));
    *group_defaulted = defaulted;
    return ret;
}

/******************************************************************************
 * GetSecurityDescriptorLength    (kernelbase.@)
 */
DWORD WINAPI GetSecurityDescriptorLength( PSECURITY_DESCRIPTOR descr )
{
    return RtlLengthSecurityDescriptor( descr );
}

/******************************************************************************
 * GetSecurityDescriptorOwner    (kernelbase.@)
 */
BOOL WINAPI GetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR descr, PSID *owner, LPBOOL owner_defaulted )
{
    BOOLEAN defaulted;
    BOOL ret = set_ntstatus( RtlGetOwnerSecurityDescriptor( descr, owner, &defaulted ));
    *owner_defaulted = defaulted;
    return ret;
}

/******************************************************************************
 *  GetSecurityDescriptorSacl    (kernelbase.@)
 */
BOOL WINAPI GetSecurityDescriptorSacl( PSECURITY_DESCRIPTOR descr, LPBOOL sacl_present, PACL *sacl,
                                       LPBOOL sacl_defaulted )
{
    BOOLEAN present, defaulted;
    BOOL ret = set_ntstatus( RtlGetSaclSecurityDescriptor( descr, &present, sacl, &defaulted ));
    *sacl_present = present;
    *sacl_defaulted = defaulted;
    return ret;
}

/******************************************************************************
 * InitializeSecurityDescriptor    (kernelbase.@)
 */
BOOL WINAPI InitializeSecurityDescriptor( PSECURITY_DESCRIPTOR descr, DWORD revision )
{
    return set_ntstatus( RtlCreateSecurityDescriptor( descr, revision ));
}

/******************************************************************************
 * IsValidSecurityDescriptor    (kernelbase.@)
 */
BOOL WINAPI IsValidSecurityDescriptor( PSECURITY_DESCRIPTOR descr )
{
    if (!RtlValidSecurityDescriptor( descr ))
        return set_ntstatus(STATUS_INVALID_SECURITY_DESCR);

    return TRUE;
}

/******************************************************************************
 * MakeAbsoluteSD    (kernelbase.@)
 */
BOOL WINAPI MakeAbsoluteSD ( PSECURITY_DESCRIPTOR rel_descr, PSECURITY_DESCRIPTOR abs_descr,
                             LPDWORD abs_size, PACL dacl, LPDWORD dacl_size, PACL sacl, LPDWORD sacl_size,
                             PSID owner, LPDWORD owner_size, PSID group, LPDWORD group_size )
{
    return set_ntstatus( RtlSelfRelativeToAbsoluteSD( rel_descr, abs_descr, abs_size,
                                                      dacl, dacl_size, sacl, sacl_size,
                                                      owner, owner_size, group, group_size ));
}

/******************************************************************************
 * MakeSelfRelativeSD    (kernelbase.@)
 */
BOOL WINAPI MakeSelfRelativeSD( PSECURITY_DESCRIPTOR abs_descr, PSECURITY_DESCRIPTOR rel_descr,
                                LPDWORD len )
{
    return set_ntstatus( RtlMakeSelfRelativeSD( abs_descr, rel_descr, len ));
}

/******************************************************************************
 * SetFileSecurityW    (kernelbase.@)
 */
BOOL WINAPI SetFileSecurityW( LPCWSTR name, SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR descr )
{
    HANDLE file;
    DWORD access = 0;
    NTSTATUS status;

    TRACE( "(%s, 0x%lx, %p)\n", debugstr_w(name), info, descr );

    if (info & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION)) access |= WRITE_OWNER;
    if (info & SACL_SECURITY_INFORMATION) access |= ACCESS_SYSTEM_SECURITY;
    if (info & DACL_SECURITY_INFORMATION) access |= WRITE_DAC;

    if (!(status = open_file( name, access, &file )))
    {
        status = NtSetSecurityObject( file, info, descr );
        NtClose( file );
    }
    return set_ntstatus( status );
}

/*************************************************************************
 * SetKernelObjectSecurity    (kernelbase.@)
 */
BOOL WINAPI SetKernelObjectSecurity( HANDLE handle, SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR descr )
{
    return set_ntstatus( NtSetSecurityObject( handle, info, descr ));
}

/*************************************************************************
 * SetPrivateObjectSecurity    (kernelbase.@)
 */
BOOL WINAPI SetPrivateObjectSecurity( SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR descr,
                                      PSECURITY_DESCRIPTOR *obj_descr, PGENERIC_MAPPING mapping,
                                      HANDLE token )
{
    FIXME( "0x%08lx %p %p %p %p - stub\n", info, descr, obj_descr, mapping, token );
    return TRUE;
}

/*************************************************************************
 * SetPrivateObjectSecurityEx    (kernelbase.@)
 */
BOOL WINAPI SetPrivateObjectSecurityEx( SECURITY_INFORMATION info, PSECURITY_DESCRIPTOR descr,
                                        PSECURITY_DESCRIPTOR *obj_descr, ULONG flags,
                                        PGENERIC_MAPPING mapping, HANDLE token )
{
    FIXME( "0x%08lx %p %p %lu %p %p - stub\n", info, descr, obj_descr, flags, mapping, token );
    return TRUE;
}

/******************************************************************************
 * SetSecurityDescriptorControl    (kernelbase.@)
 */
BOOL WINAPI SetSecurityDescriptorControl( PSECURITY_DESCRIPTOR descr, SECURITY_DESCRIPTOR_CONTROL mask,
                                          SECURITY_DESCRIPTOR_CONTROL set )
{
    return set_ntstatus( RtlSetControlSecurityDescriptor( descr, mask, set ));
}

/******************************************************************************
 *  SetSecurityDescriptorDacl    (kernelbase.@)
 */
BOOL WINAPI SetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR descr, BOOL present, PACL dacl, BOOL defaulted )
{
    return set_ntstatus( RtlSetDaclSecurityDescriptor( descr, present, dacl, defaulted ));
}

/******************************************************************************
 * SetSecurityDescriptorGroup    (kernelbase.@)
 */
BOOL WINAPI SetSecurityDescriptorGroup( PSECURITY_DESCRIPTOR descr, PSID group, BOOL defaulted )
{
    return set_ntstatus( RtlSetGroupSecurityDescriptor( descr, group, defaulted ));
}

/******************************************************************************
 * SetSecurityDescriptorOwner    (kernelbase.@)
 */
BOOL WINAPI SetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR descr, PSID owner, BOOL defaulted )
{
    return set_ntstatus( RtlSetOwnerSecurityDescriptor( descr, owner, defaulted ));
}

/**************************************************************************
 * SetSecurityDescriptorSacl    (kernelbase.@)
 */
BOOL WINAPI SetSecurityDescriptorSacl ( PSECURITY_DESCRIPTOR descr, BOOL present, PACL sacl, BOOL defaulted )
{
    return set_ntstatus( RtlSetSaclSecurityDescriptor( descr, present, sacl, defaulted ));
}


/******************************************************************************
 * Access control functions
 ******************************************************************************/


/******************************************************************************
 * AccessCheck    (kernelbase.@)
 */
BOOL WINAPI AccessCheck( PSECURITY_DESCRIPTOR descr, HANDLE token, DWORD access, PGENERIC_MAPPING mapping,
                         PPRIVILEGE_SET priv, LPDWORD priv_len, LPDWORD granted, LPBOOL status )
{
    NTSTATUS access_status;
    BOOL ret = set_ntstatus( NtAccessCheck( descr, token, access, mapping, priv, priv_len,
                                            granted, &access_status ));
    if (ret) *status = set_ntstatus( access_status );
    return ret;
}

/******************************************************************************
 * AccessCheckAndAuditAlarmW    (kernelbase.@)
 */
BOOL WINAPI AccessCheckAndAuditAlarmW( LPCWSTR subsystem, LPVOID id, LPWSTR type_name,
                                       LPWSTR name, PSECURITY_DESCRIPTOR descr, DWORD access,
                                       PGENERIC_MAPPING mapping, BOOL creation,
                                       LPDWORD granted, LPBOOL status, LPBOOL on_close )
{
    FIXME( "stub (%s,%p,%s,%s,%p,%08lx,%p,%x,%p,%p,%p)\n", debugstr_w(subsystem),
           id, debugstr_w(type_name), debugstr_w(name), descr, access, mapping,
           creation, granted, status, on_close );
    return TRUE;
}

/******************************************************************************
 * AccessCheckByType    (kernelbase.@)
 */
BOOL WINAPI AccessCheckByType( PSECURITY_DESCRIPTOR descr, PSID sid, HANDLE token, DWORD access,
                               POBJECT_TYPE_LIST types, DWORD types_len, PGENERIC_MAPPING mapping,
                               PPRIVILEGE_SET priv, LPDWORD priv_len, LPDWORD granted, LPBOOL status )
{
    FIXME("stub\n");
    *status = TRUE;
    return !*status;
}

/******************************************************************************
 * AddAccessAllowedAce    (kernelbase.@)
 */
BOOL WINAPI AddAccessAllowedAce( PACL acl, DWORD rev, DWORD access, PSID sid )
{
    return set_ntstatus( RtlAddAccessAllowedAce( acl, rev, access, sid ));
}

/******************************************************************************
 * AddAccessAllowedAceEx    (kernelbase.@)
 */
BOOL WINAPI AddAccessAllowedAceEx( PACL acl, DWORD rev, DWORD flags, DWORD access, PSID sid )
{
    return set_ntstatus( RtlAddAccessAllowedAceEx( acl, rev, flags, access, sid ));
}

/******************************************************************************
 * AddAccessAllowedObjectAce    (kernelbase.@)
 */
BOOL WINAPI AddAccessAllowedObjectAce( PACL acl, DWORD rev, DWORD flags, DWORD access,
                                       GUID *type, GUID *inherit, PSID sid )
{
    return set_ntstatus( RtlAddAccessAllowedObjectAce( acl, rev, flags, access, type, inherit, sid ));
}

/******************************************************************************
 * AddAccessDeniedAce    (kernelbase.@)
 */
BOOL WINAPI AddAccessDeniedAce( PACL acl, DWORD rev, DWORD access, PSID sid )
{
    return set_ntstatus( RtlAddAccessDeniedAce( acl, rev, access, sid ));
}

/******************************************************************************
 * AddAccessDeniedAceEx    (kernelbase.@)
 */
BOOL WINAPI AddAccessDeniedAceEx( PACL acl, DWORD rev, DWORD flags, DWORD access, PSID sid )
{
    return set_ntstatus( RtlAddAccessDeniedAceEx( acl, rev, flags, access, sid ));
}

/******************************************************************************
 * AddAccessDeniedObjectAce    (kernelbase.@)
 */
BOOL WINAPI AddAccessDeniedObjectAce( PACL acl, DWORD rev, DWORD flags, DWORD access,
                                      GUID *type, GUID *inherit, PSID sid )
{
    return set_ntstatus( RtlAddAccessDeniedObjectAce( acl, rev, flags, access, type, inherit, sid ));
}

/******************************************************************************
 * AddAce    (kernelbase.@)
 */
BOOL WINAPI AddAce( PACL acl, DWORD rev, DWORD index, LPVOID list, DWORD len )
{
    return set_ntstatus( RtlAddAce( acl, rev, index, list, len ));
}

/******************************************************************************
 * AddAuditAccessAce    (kernelbase.@)
 */
BOOL WINAPI AddAuditAccessAce( PACL acl, DWORD rev, DWORD access, PSID sid, BOOL success, BOOL failure )
{
    return set_ntstatus( RtlAddAuditAccessAce( acl, rev, access, sid, success, failure ));
}

/******************************************************************************
 * AddAuditAccessAceEx    (kernelbase.@)
 */
BOOL WINAPI AddAuditAccessAceEx( PACL acl, DWORD rev, DWORD flags, DWORD access,
                                 PSID sid, BOOL success, BOOL failure )
{
    return set_ntstatus( RtlAddAuditAccessAceEx( acl, rev, flags, access, sid, success, failure ));
}

/******************************************************************************
 * AddAuditAccessObjectAce    (kernelbase.@)
 */
BOOL WINAPI AddAuditAccessObjectAce( PACL acl, DWORD rev, DWORD flags, DWORD access,
                                     GUID *type, GUID *inherit, PSID sid, BOOL success, BOOL failure )
{
    return set_ntstatus( RtlAddAuditAccessObjectAce( acl, rev, flags, access,
                                                     type, inherit, sid, success, failure ));
}

/******************************************************************************
 * AddMandatoryAce    (kernelbase.@)
 */
BOOL WINAPI AddMandatoryAce( PACL acl, DWORD rev, DWORD flags, DWORD policy, PSID sid )
{
    return set_ntstatus( RtlAddMandatoryAce( acl, rev, flags, policy,
                                             SYSTEM_MANDATORY_LABEL_ACE_TYPE, sid ));
}

/******************************************************************************
 * AreAllAccessesGranted    (kernelbase.@)
 */
BOOL WINAPI AreAllAccessesGranted( DWORD granted, DWORD desired )
{
    return RtlAreAllAccessesGranted( granted, desired );
}

/******************************************************************************
 * AreAnyAccessesGranted    (kernelbase.@)
 */
BOOL WINAPI AreAnyAccessesGranted( DWORD granted, DWORD desired )
{
    return RtlAreAnyAccessesGranted( granted, desired );
}

/******************************************************************************
 * DeleteAce    (kernelbase.@)
 */
BOOL WINAPI DeleteAce( PACL acl, DWORD index )
{
    return set_ntstatus( RtlDeleteAce( acl, index ));
}

/******************************************************************************
 * FindFirstFreeAce    (kernelbase.@)
 */
BOOL WINAPI FindFirstFreeAce( PACL acl, LPVOID *ace)
{
    return RtlFirstFreeAce( acl, (PACE_HEADER *)ace );
}

/******************************************************************************
 * GetAce    (kernelbase.@)
 */
BOOL WINAPI GetAce( PACL acl, DWORD index, LPVOID *ace )
{
    return set_ntstatus( RtlGetAce( acl, index, ace ));
}

/******************************************************************************
 * GetAclInformation    (kernelbase.@)
 */
BOOL WINAPI GetAclInformation( PACL acl, LPVOID info, DWORD len, ACL_INFORMATION_CLASS class )
{
    return set_ntstatus( RtlQueryInformationAcl( acl, info, len, class ));
}

/*************************************************************************
 * InitializeAcl    (kernelbase.@)
 */
BOOL WINAPI InitializeAcl( PACL acl, DWORD size, DWORD rev )
{
    return set_ntstatus( RtlCreateAcl( acl, size, rev ));
}

/******************************************************************************
 * IsValidAcl    (kernelbase.@)
 */
BOOL WINAPI IsValidAcl( PACL acl )
{
    return RtlValidAcl( acl );
}

/******************************************************************************
 * MapGenericMask    (kernelbase.@)
 */
void WINAPI MapGenericMask( PDWORD access, PGENERIC_MAPPING mapping )
{
    RtlMapGenericMask( access, mapping );
}

/******************************************************************************
 * ObjectCloseAuditAlarmW    (kernelbase.@)
 */
BOOL WINAPI ObjectCloseAuditAlarmW( LPCWSTR subsystem, LPVOID id, BOOL on_close )
{
    FIXME( "stub (%s,%p,%x)\n", debugstr_w(subsystem), id, on_close );
    return TRUE;
}

/******************************************************************************
 * ObjectDeleteAuditAlarmW    (kernelbase.@)
 */
BOOL WINAPI ObjectDeleteAuditAlarmW( LPCWSTR subsystem, LPVOID id, BOOL on_close )
{
    FIXME( "stub (%s,%p,%x)\n", debugstr_w(subsystem), id, on_close );
    return TRUE;
}

/******************************************************************************
 * ObjectOpenAuditAlarmW    (kernelbase.@)
 */
BOOL WINAPI ObjectOpenAuditAlarmW( LPCWSTR subsystem, LPVOID id, LPWSTR type, LPWSTR name,
                                   PSECURITY_DESCRIPTOR descr, HANDLE token, DWORD desired,
                                   DWORD granted, PPRIVILEGE_SET privs, BOOL creation,
                                   BOOL access, LPBOOL on_close )
{
    FIXME( "stub (%s,%p,%s,%s,%p,%p,0x%08lx,0x%08lx,%p,%x,%x,%p)\n", debugstr_w(subsystem),
           id, debugstr_w(type), debugstr_w(name), descr, token, desired, granted,
           privs, creation, access, on_close );
    return TRUE;
}

/******************************************************************************
 * ObjectPrivilegeAuditAlarmW    (kernelbase.@)
 */
BOOL WINAPI ObjectPrivilegeAuditAlarmW( LPCWSTR subsystem, LPVOID id, HANDLE token,
                                        DWORD desired, PPRIVILEGE_SET privs, BOOL granted )
{
    FIXME( "stub (%s,%p,%p,0x%08lx,%p,%x)\n", debugstr_w(subsystem), id, token, desired, privs, granted );
    return TRUE;
}

/******************************************************************************
 * PrivilegedServiceAuditAlarmW    (kernelbase.@)
 */
BOOL WINAPI PrivilegedServiceAuditAlarmW( LPCWSTR subsystem, LPCWSTR service, HANDLE token,
                                          PPRIVILEGE_SET privs, BOOL granted )
{
    FIXME( "stub %s,%s,%p,%p,%x)\n", debugstr_w(subsystem), debugstr_w(service), token, privs, granted );
    return TRUE;
}

/******************************************************************************
 * SetAclInformation    (kernelbase.@)
 */
BOOL WINAPI SetAclInformation( PACL acl, LPVOID info, DWORD len, ACL_INFORMATION_CLASS class )
{
    FIXME( "%p %p 0x%08lx 0x%08x - stub\n", acl, info, len, class );
    return TRUE;
}

/******************************************************************************
 * SetCachedSigningLevel    (kernelbase.@)
 */
BOOL WINAPI SetCachedSigningLevel( PHANDLE source, ULONG count, ULONG flags, HANDLE file )
{
    FIXME( "%p %lu %lu %p - stub\n", source, count, flags, file );
    return TRUE;
}
