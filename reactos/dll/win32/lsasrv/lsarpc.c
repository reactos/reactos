/* INCLUDES ****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntsecapi.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include "lsa_s.h"

#include <wine/debug.h>

#define POLICY_DELETE (RTL_HANDLE_VALID << 1)
typedef struct _LSAR_POLICY_HANDLE
{
    ULONG Flags;
    LONG RefCount;
    ACCESS_MASK AccessGranted;
} LSAR_POLICY_HANDLE, *PLSAR_POLICY_HANDLE;

static RTL_CRITICAL_SECTION PolicyHandleTableLock;
static RTL_HANDLE_TABLE PolicyHandleTable;

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);

/* FUNCTIONS ***************************************************************/

/*static*/ NTSTATUS
ReferencePolicyHandle(IN LSAPR_HANDLE ObjectHandle,
                      IN ACCESS_MASK DesiredAccess,
                      OUT PLSAR_POLICY_HANDLE *Policy)
{
    PLSAR_POLICY_HANDLE ReferencedPolicy;
    NTSTATUS Status = STATUS_SUCCESS;

    RtlEnterCriticalSection(&PolicyHandleTableLock);

    if (RtlIsValidIndexHandle(&PolicyHandleTable,
                              (ULONG)ObjectHandle,
                              (PRTL_HANDLE_TABLE_ENTRY*)&ReferencedPolicy) &&
        !(ReferencedPolicy->Flags & POLICY_DELETE))
    {
        if (RtlAreAllAccessesGranted(ReferencedPolicy->AccessGranted,
                                     DesiredAccess))
        {
            ReferencedPolicy->RefCount++;
            *Policy = ReferencedPolicy;
        }
        else
            Status = STATUS_ACCESS_DENIED;
    }
    else
        Status = STATUS_INVALID_HANDLE;

    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    return Status;
}

/*static*/ VOID
DereferencePolicyHandle(IN OUT PLSAR_POLICY_HANDLE Policy,
                        IN BOOLEAN Delete)
{
    RtlEnterCriticalSection(&PolicyHandleTableLock);

    if (Delete)
    {
        Policy->Flags |= POLICY_DELETE;
        Policy->RefCount--;

        ASSERT(Policy->RefCount != 0);
    }

    if (--Policy->RefCount == 0)
    {
        ASSERT(Policy->Flags & POLICY_DELETE);
        RtlFreeHandle(&PolicyHandleTable,
                      (PRTL_HANDLE_TABLE_ENTRY)Policy);
    }

    RtlLeaveCriticalSection(&PolicyHandleTableLock);
}

VOID
LsarStartRpcServer(VOID)
{
    RPC_STATUS Status;

    RtlInitializeCriticalSection(&PolicyHandleTableLock);
    RtlInitializeHandleTable(0x1000,
                             sizeof(LSAR_POLICY_HANDLE),
                             &PolicyHandleTable);

    TRACE("LsarStartRpcServer() called");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    10,
                                    L"\\pipe\\lsarpc",
                                    NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerRegisterIf(lsarpc_v0_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerListen(1, 20, TRUE);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerListen() failed (Status %lx)\n", Status);
        return;
    }

    TRACE("LsarStartRpcServer() done\n");
}


/* Function 0 */
NTSTATUS LsarClose(
    handle_t hBinding,
    LSAPR_HANDLE *ObjectHandle)
{
#if 0
    PLSAR_POLICY_HANDLE Policy = NULL;
    NTSTATUS Status;

    TRACE("0x%p\n", ObjectHandle);

    Status = ReferencePolicyHandle(*ObjectHandle,
                                   0,
                                   &Policy);
    if (NT_SUCCESS(Status))
    {
        /* delete the handle */
        DereferencePolicyHandle(Policy,
                                TRUE);
    }

    return Status;
#endif
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 1 */
NTSTATUS LsarDelete(
    handle_t hBinding,
    LSAPR_HANDLE ObjectHandle)
{
    /* Deprecated */
    return STATUS_NOT_SUPPORTED;
}


/* Function 2 */
NTSTATUS LsarEnumeratePrivileges(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 3 */
NTSTATUS LsarQuerySecurityObject(
    handle_t hBinding,
    LSAPR_HANDLE ObjectHandle,
    SECURITY_INFORMATION SecurityInformation,
    PLSAPR_SR_SECURITY_DESCRIPTOR *SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 4 */
NTSTATUS LsarSetSecurityObject(
    handle_t hBinding,
    LSAPR_HANDLE ObjectHandle,
    SECURITY_INFORMATION SecurityInformation,
    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 5 */
NTSTATUS LsarChangePassword(
    handle_t hBinding,
    PRPC_UNICODE_STRING String1,
    PRPC_UNICODE_STRING String2,
    PRPC_UNICODE_STRING String3,
    PRPC_UNICODE_STRING String4,
    PRPC_UNICODE_STRING String5)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 6 */
NTSTATUS LsarOpenPolicy(
    handle_t hBinding,
    LPWSTR SystemName,
    PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *PolicyHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 7 */
NTSTATUS LsarQueryInformationPolicy(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 8 */
NTSTATUS LsarSetInformationPolicy(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 9 */
NTSTATUS LsarClearAuditLog(
    handle_t hBinding,
    LSAPR_HANDLE ObjectHandle)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 10 */
NTSTATUS LsarCreateAccount(
    handle_t hBinding,
    PRPC_SID AccountSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *AccountHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 11 */
NTSTATUS LsarEnumerateAccounts(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 12 */
NTSTATUS LsarCreateTrustedDomain(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUST_INFORMATION TrustedDomainInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 13 */
NTSTATUS LsarEnumerateTrustedDomains(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 14 */
NTSTATUS LsarLookupNames(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 15 */
NTSTATUS LsarLookupSids(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 16 */
NTSTATUS LsarCreateSecret(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 17 */
NTSTATUS LsarOpenAccount(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *AccountHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 18 */
NTSTATUS LsarEnumeratePrivilegesAccount(
    handle_t hBinding,
    LSAPR_HANDLE AccountHandle,
    PLSAPR_PRIVILEGE_SET *Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 19 */
NTSTATUS LsarAddPrivilegesToAccount(
    handle_t hBinding,
    LSAPR_HANDLE AccountHandle,
    PLSAPR_PRIVILEGE_SET Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 20 */
NTSTATUS LsarRemovePrivilegesFromAccount(
    handle_t hBinding,
    LSAPR_HANDLE AccountHandle,
    BOOL AllPrivileges,
    PLSAPR_PRIVILEGE_SET Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 21 */
NTSTATUS LsarGetQuotasForAccount(
    handle_t hBinding,
    LSAPR_HANDLE AccountHandle,
    PQUOTA_LIMITS QuotaLimits)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 22 */
NTSTATUS LsarSetQuotasForAccount(
    handle_t hBinding,
    LSAPR_HANDLE AccountHandle,
    PQUOTA_LIMITS QuotaLimits)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 23 */
NTSTATUS LsarGetSystemAccessAccount(
    handle_t hBinding,
    LSAPR_HANDLE AccountHandle,
    ACCESS_MASK *SystemAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 24 */
NTSTATUS LsarSetSystemAccessAccount(
    handle_t hBinding,
    LSAPR_HANDLE AccountHandle,
    ACCESS_MASK SystemAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 25 */
NTSTATUS LsarOpenTrustedDomain(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 26 */
NTSTATUS LsarQueryInfoTrustedDomain(
    handle_t hBinding,
    LSAPR_HANDLE TrustedDomainHandle,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 27 */
NTSTATUS LsarSetInformationTrustedDomain(
    handle_t hBinding,
    LSAPR_HANDLE TrustedDomainHandle,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 28 */
NTSTATUS LsarOpenSecret(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 29 */
NTSTATUS LsarSetSecret(
    handle_t hBinding,
    LSAPR_HANDLE *SecretHandle,
    PLSAPR_CR_CIPHER_VALUE EncryptedCurrentValue,
    PLSAPR_CR_CIPHER_VALUE EncryptedOldValue)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 30 */
NTSTATUS LsarQuerySecret(
    handle_t hBinding,
    LSAPR_HANDLE SecretHandle,
    PLSAPR_CR_CIPHER_VALUE *EncryptedCurrentValue,
    PLARGE_INTEGER CurrentValueSetTime,
    PLSAPR_CR_CIPHER_VALUE *EncryptedOldValue,
    PLARGE_INTEGER OldValueSetTime)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 31 */
NTSTATUS LsarLookupPrivilegeValue(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING Name,
    PLUID Value)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 32 */
NTSTATUS LsarLookupPrivilegeName(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PLUID Value,
    PRPC_UNICODE_STRING *Name)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 33 */
NTSTATUS LsarLookupPrivilegeDisplayName(
    handle_t hBinding,
    USHORT *LanguageReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 34 */
NTSTATUS LsarDeleteObject(
    handle_t hBinding,
    LSAPR_HANDLE *ObjectHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 35 */
NTSTATUS LsarEnumerateAccountsWithUserRight(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING UserRight,
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 36 */
NTSTATUS LsarEnmuerateAccountRights(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 37 */
NTSTATUS LsarAddAccountRights(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 38 */
NTSTATUS LsarRemoveAccountRights(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    BOOL AllRights,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 39 */
NTSTATUS LsarQueryTrustedDomainInfo(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 40 */
NTSTATUS LsarSetTrustedDomainInfo(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 41 */
NTSTATUS LsarDeleteTrustedDomain(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 42 */
NTSTATUS LsarStorePrivateData(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING KeyName,
    PLSAPR_CR_CIPHER_VALUE EncryptedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 43 */
NTSTATUS LsarRetrievePrivateData(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING KeyName,
    PLSAPR_CR_CIPHER_VALUE *EncryptedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 44 */
NTSTATUS LsarOpenPolicy2(
    handle_t hBinding,
    LPWSTR SystemName,
    PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *PolicyHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 45 */
NTSTATUS LsarGetUserName(
    handle_t hBinding,
    LPWSTR SystemName,
    PRPC_UNICODE_STRING *UserName,
    PRPC_UNICODE_STRING *DomainName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 46 */
NTSTATUS LsarQueryInformationPolicy2(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 47 */
NTSTATUS LsarSetInformationPolicy2(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 48 */
NTSTATUS LsarQueryTrustedDomainInfoByName(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 49 */
NTSTATUS LsarSetTrustedDomainInfoByName(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 50 */
NTSTATUS LsarEnumerateTrustedDomainsEx(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_TRUSTED_ENUM_BUFFER_EX EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 51 */
NTSTATUS LsarCreateTrustedDomainEx(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthentificationInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 52 */
NTSTATUS LsarSetPolicyReplicationHandle(
    handle_t hBinding,
    PLSAPR_HANDLE PolicyHandle)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 53 */
NTSTATUS LsarQueryDomainInformationPolicy(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 54 */
NTSTATUS LsarSetDomainInformationPolicy(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 55 */
NTSTATUS LsarOpenTrustedDomainByName(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 56 */
NTSTATUS LsarTestCall(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 57 */
NTSTATUS LsarLookupSids2(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 58 */
NTSTATUS LsarLookupNames2(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SID_EX TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 59 */
NTSTATUS LsarCreateTrustedDomainEx2(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL AuthentificationInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 60 */
NTSTATUS CredrWrite(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 61 */
NTSTATUS CredrRead(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 62 */
NTSTATUS CredrEnumerate(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 63 */
NTSTATUS CredrWriteDomainCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 64 */
NTSTATUS CredrReadDomainCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 65 */
NTSTATUS CredrDelete(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 66 */
NTSTATUS CredrGetTargetInfo(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 67 */
NTSTATUS CredrProfileLoaded(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 68 */
NTSTATUS LsarLookupNames3(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SID_EX2 TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 69 */
NTSTATUS CredrGetSessionTypes(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 70 */
NTSTATUS LsarRegisterAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 71 */
NTSTATUS LsarGenAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 72 */
NTSTATUS LsarUnregisterAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 73 */
NTSTATUS LsarQueryForestTrustInformation(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 74 */
NTSTATUS LsarSetForestTrustInformation(
    handle_t hBinding,
    LSAPR_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo,
    BOOL CheckOnly,
    PLSA_FOREST_TRUST_COLLISION_INFORMATION *CollisionInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 75 */
NTSTATUS CredrRename(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 76 */
NTSTATUS LsarLookupSids3(
    handle_t hBinding,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 77 */
NTSTATUS LsarLookupNames4(
    handle_t hBinding,
    handle_t RpcHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SID_EX2 TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 78 */
NTSTATUS LsarOpenPolicySce(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 79 */
NTSTATUS LsarAdtRegisterSecurityEventSource(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 80 */
NTSTATUS LsarAdtUnregisterSecurityEventSource(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 81 */
NTSTATUS LsarAdtReportSecurityEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
