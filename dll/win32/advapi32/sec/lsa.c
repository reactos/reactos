/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/advapi32/sec/lsa.c
 * PURPOSE:         Local security authority functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *      19990322 EA created
 *      19990515 EA stubs
 *      20030202 KJK compressed stubs
 *
 */
#include <advapi32.h>
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/* imported from wine 1.1.14 */
static void* ADVAPI_GetDomainName(unsigned sz, unsigned ofs)
{
    HKEY key;
    LONG ret;
    BYTE* ptr = NULL;
    UNICODE_STRING* ustr;

    static const WCHAR wVNETSUP[] = {
        'S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'S','e','r','v','i','c','e','s','\\',
        'V','x','D','\\','V','N','E','T','S','U','P','\0'};

    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, wVNETSUP, 0, KEY_READ, &key);
    if (ret == ERROR_SUCCESS)
    {
        DWORD size = 0;
        static const WCHAR wg[] = { 'W','o','r','k','g','r','o','u','p',0 };

        ret = RegQueryValueExW(key, wg, NULL, NULL, NULL, &size);
        if (ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
        {
            ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz + size);
            if (!ptr) return NULL;
            ustr = (UNICODE_STRING*)(ptr + ofs);
            ustr->MaximumLength = size;
            ustr->Buffer = (WCHAR*)(ptr + sz);
            ret = RegQueryValueExW(key, wg, NULL, NULL, (LPBYTE)ustr->Buffer, &size);
            if (ret != ERROR_SUCCESS)
            {
                HeapFree(GetProcessHeap(), 0, ptr);
                ptr = NULL;
            }   
            else ustr->Length = size - sizeof(WCHAR);
        }
        RegCloseKey(key);
    }
    if (!ptr)
    {
        static const WCHAR wDomain[] = {'D','O','M','A','I','N','\0'};
        ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                        sz + sizeof(wDomain));
        if (!ptr) return NULL;
        ustr = (UNICODE_STRING*)(ptr + ofs);
        ustr->MaximumLength = sizeof(wDomain);
        ustr->Buffer = (WCHAR*)(ptr + sz);
        ustr->Length = sizeof(wDomain) - sizeof(WCHAR);
        memcpy(ustr->Buffer, wDomain, sizeof(wDomain));
    }
    return ptr;
}


static BOOL LsapIsLocalComputer(PLSA_UNICODE_STRING ServerName)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Result;
    LPWSTR buf;

    if (ServerName == NULL || ServerName->Length == 0 || ServerName->Buffer == NULL)
        return TRUE;

    buf = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));
    Result = GetComputerNameW(buf, &dwSize);
    if (Result && (ServerName->Buffer[0] == '\\') && (ServerName->Buffer[1] == '\\'))
        ServerName += 2;
    Result = Result && !lstrcmpW(ServerName->Buffer, buf);
    HeapFree(GetProcessHeap(), 0, buf);

    return Result;
}


handle_t __RPC_USER
PLSAPR_SERVER_NAME_bind(PLSAPR_SERVER_NAME pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("PLSAPR_SERVER_NAME_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\lsarpc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
PLSAPR_SERVER_NAME_unbind(PLSAPR_SERVER_NAME pszSystemName,
                          handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("PLSAPR_SERVER_NAME_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


/*
 * @implemented
 */
NTSTATUS WINAPI
LsaClose(LSA_HANDLE ObjectHandle)
{
    NTSTATUS Status;

    TRACE("LsaClose(0x%p) called\n", ObjectHandle);

    RpcTryExcept
    {
        Status = LsarClose((PLSAPR_HANDLE)&ObjectHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


/*
 * @implemented
 */
NTSTATUS WINAPI
LsaDelete(LSA_HANDLE ObjectHandle)
{
    NTSTATUS Status;

    TRACE("LsaDelete(0x%p) called\n", ObjectHandle);

    RpcTryExcept
    {
        Status = LsarDelete((LSAPR_HANDLE)ObjectHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaAddAccountRights(
    LSA_HANDLE PolicyHandle,
    PSID AccountSid,
    PLSA_UNICODE_STRING UserRights,
    ULONG CountOfRights)
{
    FIXME("(%p,%p,%p,0x%08x) stub\n", PolicyHandle, AccountSid, UserRights, CountOfRights);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaCreateTrustedDomainEx(
    LSA_HANDLE PolicyHandle,
    PTRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    PTRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    ACCESS_MASK DesiredAccess,
    PLSA_HANDLE TrustedDomainHandle)
{
    FIXME("(%p,%p,%p,0x%08x,%p) stub\n", PolicyHandle, TrustedDomainInformation, AuthenticationInformation,
          DesiredAccess, TrustedDomainHandle);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaDeleteTrustedDomain(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid)
{
    FIXME("(%p,%p) stub\n", PolicyHandle, TrustedDomainSid);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
WINAPI
LsaEnumerateAccountRights(
    LSA_HANDLE PolicyHandle,
    PSID AccountSid,
    PLSA_UNICODE_STRING *UserRights,
    PULONG CountOfRights)
{
    LSAPR_USER_RIGHT_SET UserRightsSet;
    NTSTATUS Status;

    TRACE("(%p,%p,%p,%p) stub\n", PolicyHandle, AccountSid, UserRights, CountOfRights);

    UserRightsSet.Entries = 0;
    UserRightsSet.UserRights = NULL;

    RpcTryExcept
    {
        Status = LsarEnmuerateAccountRights((LSAPR_HANDLE)PolicyHandle,
                                            AccountSid,
                                            &UserRightsSet);

        *CountOfRights = UserRightsSet.Entries;
        *UserRights = (PUNICODE_STRING)UserRightsSet.UserRights;
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());

        if (UserRightsSet.UserRights != NULL)
        {
            MIDL_user_free(UserRightsSet.UserRights);
        }
    }
    RpcEndExcept;

    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaEnumerateAccountsWithUserRight(
    LSA_HANDLE PolicyHandle,
    OPTIONAL PLSA_UNICODE_STRING UserRights,
    PVOID *EnumerationBuffer,
    PULONG CountReturned)
{
    FIXME("(%p,%p,%p,%p) stub\n", PolicyHandle, UserRights, EnumerationBuffer, CountReturned);
    return STATUS_NO_MORE_ENTRIES;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaEnumerateTrustedDomains(
    LSA_HANDLE PolicyHandle,
    PLSA_ENUMERATION_HANDLE EnumerationContext,
    PVOID *Buffer,
    ULONG PreferedMaximumLength,
    PULONG CountReturned)
{
    FIXME("(%p,%p,%p,0x%08x,%p) stub\n", PolicyHandle, EnumerationContext,
        Buffer, PreferedMaximumLength, CountReturned);
    
    if (CountReturned) *CountReturned = 0;
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaEnumerateTrustedDomainsEx(
    LSA_HANDLE PolicyHandle,
    PLSA_ENUMERATION_HANDLE EnumerationContext,
    PVOID *Buffer,
    ULONG PreferedMaximumLength,
    PULONG CountReturned)
{
    FIXME("(%p,%p,%p,0x%08x,%p) stub\n", PolicyHandle, EnumerationContext, Buffer,
        PreferedMaximumLength, CountReturned);
    if (CountReturned) *CountReturned = 0;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
WINAPI
LsaFreeMemory(PVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    return RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
}

/*
 * @implemented
 */
NTSTATUS
WINAPI
LsaLookupNames(IN LSA_HANDLE PolicyHandle,
               IN ULONG Count,
               IN PLSA_UNICODE_STRING Names,
               OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
               OUT PLSA_TRANSLATED_SID *Sids)
{
    LSAPR_TRANSLATED_SIDS TranslatedSids;
    ULONG MappedCount = 0;
    NTSTATUS Status;

    TRACE("(%p,0x%08x,%p,%p,%p)\n", PolicyHandle, Count, Names,
          ReferencedDomains, Sids);

    RpcTryExcept
    {
        *ReferencedDomains = NULL;
        *Sids = NULL;

        TranslatedSids.Entries = Count;
        TranslatedSids.Sids = *Sids;

        Status = LsarLookupNames((LSAPR_HANDLE)PolicyHandle,
                                 Count,
                                 (PRPC_UNICODE_STRING)Names,
                                 (PLSAPR_REFERENCED_DOMAIN_LIST *)ReferencedDomains,
                                 &TranslatedSids,
                                 LsapLookupWksta,
                                 &MappedCount);

        *Sids = (PLSA_TRANSLATED_SID)TranslatedSids.Sids;
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        if (TranslatedSids.Sids != NULL)
        {
            MIDL_user_free(TranslatedSids.Sids);
        }

        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaLookupNames2(
    LSA_HANDLE PolicyHandle,
    ULONG Flags,
    ULONG Count,
    PLSA_UNICODE_STRING Names,
    PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSA_TRANSLATED_SID2 *Sids)
{
    FIXME("(%p,0x%08x,0x%08x,%p,%p,%p) stub\n", PolicyHandle, Flags,
        Count, Names, ReferencedDomains, Sids);
    if (Names != NULL && Count > 0)
    {
        *ReferencedDomains = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(LSA_REFERENCED_DOMAIN_LIST));
        *Sids = RtlAllocateHeap(RtlGetProcessHeap(), 0, Count * sizeof(LSA_TRANSLATED_SID2));
        return STATUS_SOME_NOT_MAPPED;
    }
    return STATUS_NONE_MAPPED;
}

/*
 * @unmplemented
 */
NTSTATUS
WINAPI
LsaLookupPrivilegeName(IN LSA_HANDLE PolicyHandle,
                       IN PLUID Value,
                       OUT PUNICODE_STRING *Name)
{
    PRPC_UNICODE_STRING NameBuffer = NULL;
    NTSTATUS Status;

    TRACE("(%p,%p,%p)\n", PolicyHandle, Value, Name);

    RpcTryExcept
    {
        Status = LsarLookupPrivilegeName(PolicyHandle,
                                         Value,
                                         &NameBuffer);

        *Name = (PUNICODE_STRING)NameBuffer;
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        if (NameBuffer != NULL)
            MIDL_user_free(NameBuffer);

        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
WINAPI
LsaLookupPrivilegeValue(IN LSA_HANDLE PolicyHandle,
                        IN PLSA_UNICODE_STRING Name,
                        OUT PLUID Value)
{
    LUID Luid;
    NTSTATUS Status;

    TRACE("(%p,%p,%p)\n", PolicyHandle, Name, Value);

    RpcTryExcept
    {
        Status = LsarLookupPrivilegeValue(PolicyHandle,
                                          (PRPC_UNICODE_STRING)Name,
                                          &Luid);
        if (Status == STATUS_SUCCESS)
            *Value = Luid;
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
WINAPI
LsaLookupSids(IN LSA_HANDLE PolicyHandle,
              IN ULONG Count,
              IN PSID *Sids,
              OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
              OUT PLSA_TRANSLATED_NAME *Names)
{
    LSAPR_SID_ENUM_BUFFER SidEnumBuffer;
    LSAPR_TRANSLATED_NAMES TranslatedNames;
    ULONG MappedCount = 0;
    NTSTATUS  Status;

    TRACE("(%p,%u,%p,%p,%p)\n", PolicyHandle, Count, Sids,
          ReferencedDomains, Names);

    if (Count == 0)
        return STATUS_INVALID_PARAMETER;

    SidEnumBuffer.Entries = Count;
    SidEnumBuffer.SidInfo = (PLSAPR_SID_INFORMATION)Sids;

    RpcTryExcept
    {
        *ReferencedDomains = NULL;
        *Names = NULL;

        TranslatedNames.Entries = 0;
        TranslatedNames.Names = NULL;

        Status = LsarLookupSids((LSAPR_HANDLE)PolicyHandle,
                                &SidEnumBuffer,
                                (PLSAPR_REFERENCED_DOMAIN_LIST *)ReferencedDomains,
                                &TranslatedNames,
                                LsapLookupWksta,
                                &MappedCount);

        *Names = (PLSA_TRANSLATED_NAME)TranslatedNames.Names;
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        if (TranslatedNames.Names != NULL)
        {
            MIDL_user_free(TranslatedNames.Names);
        }

        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
}

/******************************************************************************
 * LsaNtStatusToWinError
 *
 * PARAMS
 *   Status [I]
 *
 * @implemented
 */
ULONG WINAPI
LsaNtStatusToWinError(NTSTATUS Status)
{
    TRACE("(%lx)\n", Status);
    return RtlNtStatusToDosError(Status);
}

/******************************************************************************
 * LsaOpenPolicy
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 *
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaOpenPolicy(
    IN PLSA_UNICODE_STRING SystemName,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle)
{
    NTSTATUS Status;

    TRACE("LsaOpenPolicy (%s,%p,0x%08x,%p)\n",
          SystemName ? debugstr_w(SystemName->Buffer) : "(null)",
          ObjectAttributes, DesiredAccess, PolicyHandle);

    /* FIXME: RPC should take care of this */
    if (!LsapIsLocalComputer(SystemName))
        return RPC_NT_SERVER_UNAVAILABLE;

    RpcTryExcept
    {
        *PolicyHandle = NULL;

        Status = LsarOpenPolicy(SystemName ? SystemName->Buffer : NULL,
                                (PLSAPR_OBJECT_ATTRIBUTES)ObjectAttributes,
                                DesiredAccess,
                                PolicyHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    TRACE("LsaOpenPolicy() done (Status: 0x%08lx)\n", Status);

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaOpenTrustedDomainByName(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    ACCESS_MASK DesiredAccess,
    PLSA_HANDLE TrustedDomainHandle)
{
    FIXME("(%p,%p,0x%08x,%p) stub\n", PolicyHandle, TrustedDomainName, DesiredAccess, TrustedDomainHandle);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaQueryDomainInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    PVOID *Buffer)
{
    FIXME("(%p,0x%08x,%p)\n", PolicyHandle, InformationClass, Buffer);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaQueryForestTrustInformation(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    PLSA_FOREST_TRUST_INFORMATION * ForestTrustInfo)
{
    FIXME("(%p,%p,%p) stub\n", PolicyHandle, TrustedDomainName, ForestTrustInfo);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS WINAPI
LsaQueryInformationPolicy(LSA_HANDLE PolicyHandle,
              POLICY_INFORMATION_CLASS InformationClass,
              PVOID *Buffer)
{
    TRACE("(%p,0x%08x,%p)\n", PolicyHandle, InformationClass, Buffer);

    if(!Buffer) return STATUS_INVALID_PARAMETER;
    switch (InformationClass)
    {
        case PolicyAuditEventsInformation: /* 2 */
        {
            PPOLICY_AUDIT_EVENTS_INFO p = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                                    sizeof(POLICY_AUDIT_EVENTS_INFO));
            p->AuditingMode = FALSE; /* no auditing */
            *Buffer = p;
        }
        break;
        case PolicyPrimaryDomainInformation: /* 3 */
        {
            /* Only the domain name is valid for the local computer.
             * All other fields are zero.
             */
            PPOLICY_PRIMARY_DOMAIN_INFO pinfo;

            pinfo = ADVAPI_GetDomainName(sizeof(*pinfo), offsetof(POLICY_PRIMARY_DOMAIN_INFO, Name));

            TRACE("setting domain to %s\n", debugstr_w(pinfo->Name.Buffer));

            *Buffer = pinfo;
        }
        case PolicyAccountDomainInformation: /* 5 */
        {
            struct di
            {
                POLICY_ACCOUNT_DOMAIN_INFO info;
                SID sid;
                DWORD padding[3];
                WCHAR domain[MAX_COMPUTERNAME_LENGTH + 1];
            };

            DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
            struct di * xdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*xdi));

            xdi->info.DomainName.MaximumLength = dwSize * sizeof(WCHAR);
            xdi->info.DomainName.Buffer = xdi->domain;
            if (GetComputerNameW(xdi->info.DomainName.Buffer, &dwSize))
                xdi->info.DomainName.Length = dwSize * sizeof(WCHAR);

            TRACE("setting name to %s\n", debugstr_w(xdi->info.DomainName.Buffer));

            xdi->info.DomainSid = &xdi->sid;

            /* read the computer SID from the registry */
            if (!ADVAPI_GetComputerSid(&xdi->sid))
            {
                HeapFree(GetProcessHeap(), 0, xdi);

                WARN("Computer SID not found\n");

                return STATUS_UNSUCCESSFUL;
            }

            *Buffer = xdi;
        }
        break;
        case  PolicyDnsDomainInformation:	/* 12 (0xc) */
        {
            /* Only the domain name is valid for the local computer.
             * All other fields are zero.
             */
            PPOLICY_DNS_DOMAIN_INFO pinfo;

            pinfo = ADVAPI_GetDomainName(sizeof(*pinfo), offsetof(POLICY_DNS_DOMAIN_INFO, Name));

            TRACE("setting domain to %s\n", debugstr_w(pinfo->Name.Buffer));

            *Buffer = pinfo;
        }
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
        {
            FIXME("category not implemented\n");
            return STATUS_UNSUCCESSFUL;
        }
    }
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaQueryTrustedDomainInfoByName(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PVOID *Buffer)
{
    FIXME("(%p,%p,%d,%p) stub\n", PolicyHandle, TrustedDomainName, InformationClass, Buffer);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaQueryTrustedDomainInfo(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PVOID *Buffer)
{
    FIXME("(%p,%p,%d,%p) stub\n", PolicyHandle, TrustedDomainSid, InformationClass, Buffer);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaRemoveAccountRights(
    LSA_HANDLE PolicyHandle,
    PSID AccountSid,
    BOOLEAN AllRights,
    PLSA_UNICODE_STRING UserRights,
    ULONG CountOfRights)
{
    FIXME("(%p,%p,%d,%p,0x%08x) stub\n", PolicyHandle, AccountSid, AllRights, UserRights, CountOfRights);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaRetrievePrivateData(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING KeyName,
    PLSA_UNICODE_STRING *PrivateData)
{
    FIXME("(%p,%p,%p) stub\n", PolicyHandle, KeyName, PrivateData);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaSetDomainInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    PVOID Buffer)
{
    FIXME("(%p,0x%08x,%p) stub\n", PolicyHandle, InformationClass, Buffer);
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaSetInformationPolicy(
    LSA_HANDLE PolicyHandle,
    POLICY_INFORMATION_CLASS InformationClass,
    PVOID Buffer)
{
    FIXME("(%p,0x%08x,%p) stub\n", PolicyHandle, InformationClass, Buffer);
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS WINAPI LsaSetSecret(
    IN LSA_HANDLE SecretHandle,
    IN PLSA_UNICODE_STRING EncryptedCurrentValue,
    IN PLSA_UNICODE_STRING EncryptedOldValue)
{
    FIXME("(%p,%p,%p) stub\n", SecretHandle, EncryptedCurrentValue,
            EncryptedOldValue);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaSetForestTrustInformation(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo,
    BOOL CheckOnly,
    PLSA_FOREST_TRUST_COLLISION_INFORMATION *CollisionInfo)
{
    FIXME("(%p,%p,%p,%d,%p) stub\n", PolicyHandle, TrustedDomainName, ForestTrustInfo, CheckOnly, CollisionInfo);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaSetTrustedDomainInfoByName(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING TrustedDomainName,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PVOID Buffer)
{
    FIXME("(%p,%p,%d,%p) stub\n", PolicyHandle, TrustedDomainName, InformationClass, Buffer);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS WINAPI LsaRegisterPolicyChangeNotification(
    POLICY_NOTIFICATION_INFORMATION_CLASS class,
    HANDLE event)
{
    FIXME("(%d,%p) stub\n", class, event);
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaSetTrustedDomainInformation(
    LSA_HANDLE PolicyHandle,
    PSID TrustedDomainSid,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PVOID Buffer)
{
    FIXME("(%p,%p,%d,%p) stub\n", PolicyHandle, TrustedDomainSid, InformationClass, Buffer);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaStorePrivateData(
    LSA_HANDLE PolicyHandle,
    PLSA_UNICODE_STRING KeyName,
    PLSA_UNICODE_STRING PrivateData)
{
    FIXME("(%p,%p,%p) stub\n", PolicyHandle, KeyName, PrivateData);
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/*
 * @unimplemented
 */
NTSTATUS WINAPI LsaUnregisterPolicyChangeNotification(
    POLICY_NOTIFICATION_INFORMATION_CLASS class,
    HANDLE event)
{
    FIXME("(%d,%p) stub\n", class, event);
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaGetUserName(
    PUNICODE_STRING *UserName,
    PUNICODE_STRING *DomainName)
{
    FIXME("(%p,%p) stub\n", UserName, DomainName);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaQueryInfoTrustedDomain (DWORD Unknonw0,
			   DWORD Unknonw1,
			   DWORD Unknonw2)
{
    FIXME("(%d,%d,%d) stub\n", Unknonw0, Unknonw1, Unknonw2);
    return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
