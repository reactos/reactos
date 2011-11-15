/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lsarpc.h
 * PURPOSE:         RPC interface functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"


static RTL_CRITICAL_SECTION PolicyHandleTableLock;

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


/* FUNCTIONS ***************************************************************/


VOID
LsarStartRpcServer(VOID)
{
    RPC_STATUS Status;

    RtlInitializeCriticalSection(&PolicyHandleTableLock);

    TRACE("LsarStartRpcServer() called\n");

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


void __RPC_USER LSAPR_HANDLE_rundown(LSAPR_HANDLE hHandle)
{

}


/* Function 0 */
NTSTATUS WINAPI LsarClose(
    LSAPR_HANDLE *ObjectHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("0x%p\n", ObjectHandle);

//    RtlEnterCriticalSection(&PolicyHandleTableLock);

    Status = LsapValidateDbObject(*ObjectHandle,
                                  LsaDbIgnoreObject,
                                  0);
    if (Status == STATUS_SUCCESS)
    {
        Status = LsapCloseDbObject(*ObjectHandle);
        *ObjectHandle = NULL;
    }

//    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    return Status;
}


/* Function 1 */
NTSTATUS WINAPI LsarDelete(
    LSAPR_HANDLE ObjectHandle)
{
    /* Deprecated */
    return STATUS_NOT_SUPPORTED;
}


/* Function 2 */
NTSTATUS WINAPI LsarEnumeratePrivileges(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 3 */
NTSTATUS WINAPI LsarQuerySecurityObject(
    LSAPR_HANDLE ObjectHandle,
    SECURITY_INFORMATION SecurityInformation,
    PLSAPR_SR_SECURITY_DESCRIPTOR *SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 4 */
NTSTATUS WINAPI LsarSetSecurityObject(
    LSAPR_HANDLE ObjectHandle,
    SECURITY_INFORMATION SecurityInformation,
    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 5 */
NTSTATUS WINAPI LsarChangePassword(
    handle_t IDL_handle,
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
NTSTATUS WINAPI LsarOpenPolicy(
    LPWSTR SystemName,
    PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *PolicyHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("LsarOpenPolicy called!\n");

    RtlEnterCriticalSection(&PolicyHandleTableLock);

    *PolicyHandle = LsapCreateDbObject(NULL,
                                       L"Policy",
                                       TRUE,
                                       LsaDbPolicyObject,
                                       DesiredAccess);
    if (*PolicyHandle == NULL)
        Status = STATUS_INSUFFICIENT_RESOURCES;

    RtlLeaveCriticalSection(&PolicyHandleTableLock);

    TRACE("LsarOpenPolicy done!\n");

    return Status;
}


/* Function 7 */
NTSTATUS WINAPI LsarQueryInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION *PolicyInformation)
{
    NTSTATUS Status;

    TRACE("LsarQueryInformationPolicy(%p,0x%08x,%p)\n",
          PolicyHandle, InformationClass, PolicyInformation);

    if (PolicyInformation)
    {
        TRACE("*PolicyInformation %p\n", *PolicyInformation);
    }

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  0); /* FIXME */
    if (!NT_SUCCESS(Status))
        return Status;

    switch (InformationClass)
    {
        case PolicyAuditEventsInformation:   /* 2 */
            Status = LsarQueryAuditEvents(PolicyHandle,
                                          PolicyInformation);
            break;

        case PolicyPrimaryDomainInformation: /* 3 */
            Status = LsarQueryPrimaryDomain(PolicyHandle,
                                            PolicyInformation);
            break;

        case PolicyAccountDomainInformation: /* 5 */
            Status = LsarQueryAccountDomain(PolicyHandle,
                                            PolicyInformation);
            break;

        case PolicyDnsDomainInformation:     /* 12 (0xc) */
            Status = LsarQueryDnsDomain(PolicyHandle,
                                        PolicyInformation);
            break;

        case PolicyAuditLogInformation:
        case PolicyPdAccountInformation:
        case PolicyLsaServerRoleInformation:
        case PolicyReplicaSourceInformation:
        case PolicyDefaultQuotaInformation:
        case PolicyModificationInformation:
        case PolicyAuditFullSetInformation:
        case PolicyAuditFullQueryInformation:
        case PolicyEfsInformation:
            FIXME("category not implemented\n");
            Status = STATUS_UNSUCCESSFUL;
            break;
    }

    return Status;
}


/* Function 8 */
NTSTATUS WINAPI LsarSetInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION PolicyInformation)
{
    NTSTATUS Status;

    TRACE("LsarSetInformationPolicy(%p,0x%08x,%p)\n",
          PolicyHandle, InformationClass, PolicyInformation);

    if (PolicyInformation)
    {
        TRACE("*PolicyInformation %p\n", *PolicyInformation);
    }

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  0); /* FIXME */
    if (!NT_SUCCESS(Status))
        return Status;

    switch (InformationClass)
    {
        case PolicyAuditEventsInformation:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case PolicyPrimaryDomainInformation:
            Status = LsarSetPrimaryDomain(PolicyHandle,
                                          (PLSAPR_POLICY_PRIMARY_DOM_INFO)PolicyInformation);
            break;

        case PolicyAccountDomainInformation:
            Status = LsarSetAccountDomain(PolicyHandle,
                                          (PLSAPR_POLICY_ACCOUNT_DOM_INFO)PolicyInformation);
            break;

        case PolicyDnsDomainInformation:
            Status = LsarSetDnsDomain(PolicyHandle,
                                      (PLSAPR_POLICY_DNS_DOMAIN_INFO)PolicyInformation);
            break;

        case PolicyLsaServerRoleInformation:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;
}


/* Function 9 */
NTSTATUS WINAPI LsarClearAuditLog(
    LSAPR_HANDLE ObjectHandle)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 10 */
NTSTATUS WINAPI LsarCreateAccount(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *AccountHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 11 */
NTSTATUS WINAPI LsarEnumerateAccounts(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 12 */
NTSTATUS WINAPI LsarCreateTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_TRUST_INFORMATION TrustedDomainInformation,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 13 */
NTSTATUS WINAPI LsarEnumerateTrustedDomains(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 14 */
NTSTATUS WINAPI LsarLookupNames(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount)
{
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = {SECURITY_NT_AUTHORITY};
    static const UNICODE_STRING DomainName = RTL_CONSTANT_STRING(L"DOMAIN");
    PLSAPR_REFERENCED_DOMAIN_LIST OutputDomains = NULL;
    PLSA_TRANSLATED_SID OutputSids = NULL;
    ULONG OutputSidsLength;
    ULONG i;
    PSID Sid;
    ULONG SidLength;
    NTSTATUS Status;

    TRACE("LsarLookupNames(%p, %lu, %p, %p, %p, %d, %p)\n",
          PolicyHandle, Count, Names, ReferencedDomains, TranslatedSids,
          LookupLevel, MappedCount);

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = NULL;
    *ReferencedDomains = NULL;

    OutputSidsLength = Count * sizeof(LSA_TRANSLATED_SID);
    OutputSids = MIDL_user_allocate(OutputSidsLength);
    if (OutputSids == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(OutputSids, OutputSidsLength);

    OutputDomains = MIDL_user_allocate(sizeof(LSAPR_REFERENCED_DOMAIN_LIST));
    if (OutputDomains == NULL)
    {
        MIDL_user_free(OutputSids);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    OutputDomains->Entries = Count;
    OutputDomains->Domains = MIDL_user_allocate(Count * sizeof(LSA_TRUST_INFORMATION));
    if (OutputDomains->Domains == NULL)
    {
        MIDL_user_free(OutputDomains);
        MIDL_user_free(OutputSids);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &Sid);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(OutputDomains->Domains);
        MIDL_user_free(OutputDomains);
        MIDL_user_free(OutputSids);
        return Status;
    }

    SidLength = RtlLengthSid(Sid);

    for (i = 0; i < Count; i++)
    {
        OutputDomains->Domains[i].Sid = MIDL_user_allocate(SidLength);
        RtlCopyMemory(OutputDomains->Domains[i].Sid, Sid, SidLength);

        OutputDomains->Domains[i].Name.Buffer = MIDL_user_allocate(DomainName.MaximumLength);
        OutputDomains->Domains[i].Name.Length = DomainName.Length;
        OutputDomains->Domains[i].Name.MaximumLength = DomainName.MaximumLength;
        RtlCopyMemory(OutputDomains->Domains[i].Name.Buffer, DomainName.Buffer, DomainName.MaximumLength);
    }

    for (i = 0; i < Count; i++)
    {
        OutputSids[i].Use = SidTypeWellKnownGroup;
        OutputSids[i].RelativeId = DOMAIN_USER_RID_ADMIN; //DOMAIN_ALIAS_RID_ADMINS;
        OutputSids[i].DomainIndex = i;
    }

    *ReferencedDomains = OutputDomains;

    *MappedCount = Count;

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = OutputSids;

    return STATUS_SUCCESS;
}


/* Function 15 */
NTSTATUS WINAPI LsarLookupSids(
    LSAPR_HANDLE PolicyHandle,
    PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_NAMES TranslatedNames,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount)
{
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = {SECURITY_NT_AUTHORITY};
    static const UNICODE_STRING DomainName = RTL_CONSTANT_STRING(L"DOMAIN");
    PLSAPR_REFERENCED_DOMAIN_LIST OutputDomains = NULL;
    PLSAPR_TRANSLATED_NAME OutputNames = NULL;
    ULONG OutputNamesLength;
    ULONG i;
    PSID Sid;
    ULONG SidLength;
    NTSTATUS Status;

    TRACE("LsarLookupSids(%p, %p, %p, %p, %d, %p)\n",
          PolicyHandle, SidEnumBuffer, ReferencedDomains, TranslatedNames,
          LookupLevel, MappedCount);

    TranslatedNames->Entries = SidEnumBuffer->Entries;
    TranslatedNames->Names = NULL;
    *ReferencedDomains = NULL;

    OutputNamesLength = SidEnumBuffer->Entries * sizeof(LSA_TRANSLATED_NAME);
    OutputNames = MIDL_user_allocate(OutputNamesLength);
    if (OutputNames == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(OutputNames, OutputNamesLength);

    OutputDomains = MIDL_user_allocate(sizeof(LSAPR_REFERENCED_DOMAIN_LIST));
    if (OutputDomains == NULL)
    {
        MIDL_user_free(OutputNames);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    OutputDomains->Entries = SidEnumBuffer->Entries;
    OutputDomains->Domains = MIDL_user_allocate(SidEnumBuffer->Entries * sizeof(LSA_TRUST_INFORMATION));
    if (OutputDomains->Domains == NULL)
    {
        MIDL_user_free(OutputDomains);
        MIDL_user_free(OutputNames);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &Sid);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(OutputDomains->Domains);
        MIDL_user_free(OutputDomains);
        MIDL_user_free(OutputNames);
        return Status;
    }

    SidLength = RtlLengthSid(Sid);

    for (i = 0; i < SidEnumBuffer->Entries; i++)
    {
        OutputDomains->Domains[i].Sid = MIDL_user_allocate(SidLength);
        RtlCopyMemory(OutputDomains->Domains[i].Sid, Sid, SidLength);

        OutputDomains->Domains[i].Name.Buffer = MIDL_user_allocate(DomainName.MaximumLength);
        OutputDomains->Domains[i].Name.Length = DomainName.Length;
        OutputDomains->Domains[i].Name.MaximumLength = DomainName.MaximumLength;
        RtlCopyMemory(OutputDomains->Domains[i].Name.Buffer, DomainName.Buffer, DomainName.MaximumLength);
    }

    Status = LsapLookupSids(SidEnumBuffer,
                            OutputNames);

    *ReferencedDomains = OutputDomains;

    *MappedCount = SidEnumBuffer->Entries;

    TranslatedNames->Entries = SidEnumBuffer->Entries;
    TranslatedNames->Names = OutputNames;

    return Status;
}


/* Function 16 */
NTSTATUS WINAPI LsarCreateSecret(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 17 */
NTSTATUS WINAPI LsarOpenAccount(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *AccountHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 18 */
NTSTATUS WINAPI LsarEnumeratePrivilegesAccount(
    LSAPR_HANDLE AccountHandle,
    PLSAPR_PRIVILEGE_SET *Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 19 */
NTSTATUS WINAPI LsarAddPrivilegesToAccount(
    LSAPR_HANDLE AccountHandle,
    PLSAPR_PRIVILEGE_SET Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 20 */
NTSTATUS WINAPI LsarRemovePrivilegesFromAccount(
    LSAPR_HANDLE AccountHandle,
    BOOL AllPrivileges,
    PLSAPR_PRIVILEGE_SET Privileges)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 21 */
NTSTATUS WINAPI LsarGetQuotasForAccount(
    LSAPR_HANDLE AccountHandle,
    PQUOTA_LIMITS QuotaLimits)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 22 */
NTSTATUS WINAPI LsarSetQuotasForAccount(
    LSAPR_HANDLE AccountHandle,
    PQUOTA_LIMITS QuotaLimits)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 23 */
NTSTATUS WINAPI LsarGetSystemAccessAccount(
    LSAPR_HANDLE AccountHandle,
    ACCESS_MASK *SystemAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 24 */
NTSTATUS WINAPI LsarSetSystemAccessAccount(
    LSAPR_HANDLE AccountHandle,
    ACCESS_MASK SystemAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 25 */
NTSTATUS WINAPI LsarOpenTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 26 */
NTSTATUS WINAPI LsarQueryInfoTrustedDomain(
    LSAPR_HANDLE TrustedDomainHandle,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 27 */
NTSTATUS WINAPI LsarSetInformationTrustedDomain(
    LSAPR_HANDLE TrustedDomainHandle,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 28 */
NTSTATUS WINAPI LsarOpenSecret(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING SecretName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *SecretHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 29 */
NTSTATUS WINAPI LsarSetSecret(
    LSAPR_HANDLE *SecretHandle,
    PLSAPR_CR_CIPHER_VALUE EncryptedCurrentValue,
    PLSAPR_CR_CIPHER_VALUE EncryptedOldValue)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 30 */
NTSTATUS WINAPI LsarQuerySecret(
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
NTSTATUS WINAPI LsarLookupPrivilegeValue(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING Name,
    PLUID Value)
{
    NTSTATUS Status;

    TRACE("LsarLookupPrivilegeValue(%p, %wZ, %p)\n",
          PolicyHandle, Name, Value);

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  0); /* FIXME */
    if (!NT_SUCCESS(Status))
    {
        ERR("Invalid handle (Status %lx)\n", Status);
        return Status;
    }

    TRACE("Privilege: %wZ\n", Name);

    Status = LsarpLookupPrivilegeValue((PUNICODE_STRING)Name,
                                       Value);

    return Status;
}


/* Function 32 */
NTSTATUS WINAPI LsarLookupPrivilegeName(
    LSAPR_HANDLE PolicyHandle,
    PLUID Value,
    PRPC_UNICODE_STRING *Name)
{
    NTSTATUS Status;

    TRACE("LsarLookupPrivilegeName(%p, %p, %p)\n",
          PolicyHandle, Value, Name);

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  0); /* FIXME */
    if (!NT_SUCCESS(Status))
    {
        ERR("Invalid handle\n");
        return Status;
    }

    Status = LsarpLookupPrivilegeName(Value, (PUNICODE_STRING*)Name);

    return Status;
}


/* Function 33 */
NTSTATUS WINAPI LsarLookupPrivilegeDisplayName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING Name,
    USHORT ClientLanguage,
    USHORT ClientSystemDefaultLanguage,
    PRPC_UNICODE_STRING *DisplayName,
    USHORT *LanguageReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 34 */
NTSTATUS WINAPI LsarDeleteObject(
    LSAPR_HANDLE *ObjectHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 35 */
NTSTATUS WINAPI LsarEnumerateAccountsWithUserRight(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING UserRight,
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 36 */
NTSTATUS WINAPI LsarEnmuerateAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    NTSTATUS Status;

    FIXME("(%p,%p,%p) stub\n", PolicyHandle, AccountSid, UserRights);

    Status = LsapValidateDbObject(PolicyHandle,
                                  LsaDbPolicyObject,
                                  0); /* FIXME */
    if (!NT_SUCCESS(Status))
        return Status;

    UserRights->Entries = 0;
    UserRights->UserRights = NULL;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}


/* Function 37 */
NTSTATUS WINAPI LsarAddAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 38 */
NTSTATUS WINAPI LsarRemoveAccountRights(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID AccountSid,
    BOOL AllRights,
    PLSAPR_USER_RIGHT_SET UserRights)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 39 */
NTSTATUS WINAPI LsarQueryTrustedDomainInfo(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 40 */
NTSTATUS WINAPI LsarSetTrustedDomainInfo(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 41 */
NTSTATUS WINAPI LsarDeleteTrustedDomain(
    LSAPR_HANDLE PolicyHandle,
    PRPC_SID TrustedDomainSid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 42 */
NTSTATUS WINAPI LsarStorePrivateData(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING KeyName,
    PLSAPR_CR_CIPHER_VALUE EncryptedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 43 */
NTSTATUS WINAPI LsarRetrievePrivateData(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING KeyName,
    PLSAPR_CR_CIPHER_VALUE *EncryptedData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 44 */
NTSTATUS WINAPI LsarOpenPolicy2(
    LPWSTR SystemName,
    PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *PolicyHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 45 */
NTSTATUS WINAPI LsarGetUserName(
    LPWSTR SystemName,
    PRPC_UNICODE_STRING *UserName,
    PRPC_UNICODE_STRING *DomainName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 46 */
NTSTATUS WINAPI LsarQueryInformationPolicy2(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 47 */
NTSTATUS WINAPI LsarSetInformationPolicy2(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 48 */
NTSTATUS WINAPI LsarQueryTrustedDomainInfoByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 49 */
NTSTATUS WINAPI LsarSetTrustedDomainInfoByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 50 */
NTSTATUS WINAPI LsarEnumerateTrustedDomainsEx(
    LSAPR_HANDLE PolicyHandle,
    DWORD *EnumerationContext,
    PLSAPR_TRUSTED_ENUM_BUFFER_EX EnumerationBuffer,
    DWORD PreferedMaximumLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 51 */
NTSTATUS WINAPI LsarCreateTrustedDomainEx(
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
NTSTATUS WINAPI LsarSetPolicyReplicationHandle(
    PLSAPR_HANDLE PolicyHandle)
{
    /* Deprecated */
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 53 */
NTSTATUS WINAPI LsarQueryDomainInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long *PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 54 */
NTSTATUS WINAPI LsarSetDomainInformationPolicy(
    LSAPR_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    unsigned long PolicyInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 55 */
NTSTATUS WINAPI LsarOpenTrustedDomainByName(
    LSAPR_HANDLE PolicyHandle,
    PRPC_UNICODE_STRING TrustedDomainName,
    ACCESS_MASK DesiredAccess,
    LSAPR_HANDLE *TrustedDomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 56 */
NTSTATUS WINAPI LsarTestCall(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 57 */
NTSTATUS WINAPI LsarLookupSids2(
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
NTSTATUS WINAPI LsarLookupNames2(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS_EX TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 59 */
NTSTATUS WINAPI LsarCreateTrustedDomainEx2(
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
NTSTATUS WINAPI CredrWrite(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 61 */
NTSTATUS WINAPI CredrRead(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 62 */
NTSTATUS WINAPI CredrEnumerate(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 63 */
NTSTATUS WINAPI CredrWriteDomainCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 64 */
NTSTATUS WINAPI CredrReadDomainCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 65 */
NTSTATUS WINAPI CredrDelete(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 66 */
NTSTATUS WINAPI CredrGetTargetInfo(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 67 */
NTSTATUS WINAPI CredrProfileLoaded(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 68 */
NTSTATUS WINAPI LsarLookupNames3(
    LSAPR_HANDLE PolicyHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = {SECURITY_NT_AUTHORITY};
    static const UNICODE_STRING DomainName = RTL_CONSTANT_STRING(L"DOMAIN");
    PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer = NULL;
    PLSAPR_TRANSLATED_SID_EX2 SidsBuffer = NULL;
    ULONG SidsBufferLength;
    ULONG DomainSidLength;
    ULONG AccountSidLength;
    PSID DomainSid;
    PSID AccountSid;
    ULONG i;
    NTSTATUS Status;

    TRACE("LsarLookupNames3(%p, %lu, %p, %p, %p, %d, %p, %lu, %lu)\n",
          PolicyHandle, Count, Names, ReferencedDomains, TranslatedSids,
          LookupLevel, MappedCount, LookupOptions, ClientRevision);

    if (Count == 0)
        return STATUS_NONE_MAPPED;

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = NULL;
    *ReferencedDomains = NULL;

    SidsBufferLength = Count * sizeof(LSAPR_TRANSLATED_SID_EX2);
    SidsBuffer = MIDL_user_allocate(SidsBufferLength);
    if (SidsBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (i = 0; i < Count; i++)
    {
        SidsBuffer[i].Use = SidTypeUser;
        SidsBuffer[i].Sid = NULL;
        SidsBuffer[i].DomainIndex = -1;
        SidsBuffer[i].Flags = 0;
    }

    DomainsBuffer = MIDL_user_allocate(sizeof(LSAPR_REFERENCED_DOMAIN_LIST));
    if (DomainsBuffer == NULL)
    {
        MIDL_user_free(SidsBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DomainsBuffer->Entries = Count;
    DomainsBuffer->Domains = MIDL_user_allocate(Count * sizeof(LSA_TRUST_INFORMATION));
    if (DomainsBuffer->Domains == NULL)
    {
        MIDL_user_free(DomainsBuffer);
        MIDL_user_free(SidsBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                         2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &DomainSid);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(DomainsBuffer->Domains);
        MIDL_user_free(DomainsBuffer);
        MIDL_user_free(SidsBuffer);
        return Status;
    }

    DomainSidLength = RtlLengthSid(DomainSid);

    for (i = 0; i < Count; i++)
    {
        DomainsBuffer->Domains[i].Sid = MIDL_user_allocate(DomainSidLength);
        RtlCopyMemory(DomainsBuffer->Domains[i].Sid,
                      DomainSid,
                      DomainSidLength);

        DomainsBuffer->Domains[i].Name.Buffer = MIDL_user_allocate(DomainName.MaximumLength);
        DomainsBuffer->Domains[i].Name.Length = DomainName.Length;
        DomainsBuffer->Domains[i].Name.MaximumLength = DomainName.MaximumLength;
        RtlCopyMemory(DomainsBuffer->Domains[i].Name.Buffer,
                      DomainName.Buffer,
                      DomainName.MaximumLength);
    }

    Status = RtlAllocateAndInitializeSid(&IdentifierAuthority,
                                         3,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         DOMAIN_USER_RID_ADMIN,
                                         0, 0, 0, 0, 0,
                                         &AccountSid);
    if (!NT_SUCCESS(Status))
    {
        MIDL_user_free(DomainsBuffer->Domains);
        MIDL_user_free(DomainsBuffer);
        MIDL_user_free(SidsBuffer);
        return Status;
    }

    AccountSidLength = RtlLengthSid(AccountSid);

    for (i = 0; i < Count; i++)
    {
        SidsBuffer[i].Use = SidTypeWellKnownGroup;
        SidsBuffer[i].Sid = MIDL_user_allocate(AccountSidLength);

        RtlCopyMemory(SidsBuffer[i].Sid,
                      AccountSid,
                      AccountSidLength);

        SidsBuffer[i].DomainIndex = i;
        SidsBuffer[i].Flags = 0;
    }

    *ReferencedDomains = DomainsBuffer;
    *MappedCount = Count;

    TranslatedSids->Entries = Count;
    TranslatedSids->Sids = SidsBuffer;

    return STATUS_SUCCESS;
}


/* Function 69 */
NTSTATUS WINAPI CredrGetSessionTypes(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 70 */
NTSTATUS WINAPI LsarRegisterAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 71 */
NTSTATUS WINAPI LsarGenAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 72 */
NTSTATUS WINAPI LsarUnregisterAuditEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 73 */
NTSTATUS WINAPI LsarQueryForestTrustInformation(
    LSAPR_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 74 */
NTSTATUS WINAPI LsarSetForestTrustInformation(
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
NTSTATUS WINAPI CredrRename(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 76 */
NTSTATUS WINAPI LsarLookupSids3(
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


/* Function 77 */
NTSTATUS WINAPI LsarLookupNames4(
    handle_t RpcHandle,
    DWORD Count,
    PRPC_UNICODE_STRING Names,
    PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    LSAP_LOOKUP_LEVEL LookupLevel,
    DWORD *MappedCount,
    DWORD LookupOptions,
    DWORD ClientRevision)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 78 */
NTSTATUS WINAPI LsarOpenPolicySce(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 79 */
NTSTATUS WINAPI LsarAdtRegisterSecurityEventSource(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 80 */
NTSTATUS WINAPI LsarAdtUnregisterSecurityEventSource(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 81 */
NTSTATUS WINAPI LsarAdtReportSecurityEvent(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 82 */
NTSTATUS WINAPI CredrFindBestCredential(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 83 */
NTSTATUS WINAPI LsarSetAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 84 */
NTSTATUS WINAPI LsarQueryAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 85 */
NTSTATUS WINAPI LsarEnumerateAuditPolicy(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 86 */
NTSTATUS WINAPI LsarEnumerateAuditCategories(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 87 */
NTSTATUS WINAPI LsarEnumerateAuditSubCategories(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 88 */
NTSTATUS WINAPI LsarLookupAuditCategoryName(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 89 */
NTSTATUS WINAPI LsarLookupAuditSubCategoryName(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 90 */
NTSTATUS WINAPI LsarSetAuditSecurity(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 91 */
NTSTATUS WINAPI LsarQueryAuditSecurity(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 92 */
NTSTATUS WINAPI CredReadByTokenHandle(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 93 */
NTSTATUS WINAPI CredrRestoreCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/* Function 94 */
NTSTATUS WINAPI CredrBackupCredentials(
    handle_t hBinding)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
