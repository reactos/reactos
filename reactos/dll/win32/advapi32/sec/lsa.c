/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/lsa.c
 * PURPOSE:         Local security authority functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *	19990322 EA created
 *	19990515 EA stubs
 *      20030202 KJK compressed stubs
 *
 */

#include <advapi32.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);


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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaEnumerateAccountRights(
    LSA_HANDLE PolicyHandle,
    PSID AccountSid,
    PLSA_UNICODE_STRING *UserRights,
    PULONG CountOfRights)
{
    FIXME("(%p,%p,%p,%p) stub\n", PolicyHandle, AccountSid, UserRights, CountOfRights);
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS WINAPI
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
LsaLookupNames(
    LSA_HANDLE PolicyHandle,
    ULONG Count,
    PLSA_UNICODE_STRING Names,
    PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSA_TRANSLATED_SID *Sids)
{
    PLSA_TRANSLATED_SID2 Sids2;
    LSA_TRANSLATED_SID *TranslatedSids;
    ULONG i;
    NTSTATUS Status;

    TRACE("(%p,0x%08x,%p,%p,%p)\n", PolicyHandle, Count, Names,
          ReferencedDomains, Sids);

    /* Call LsaLookupNames2, which supersedes this function */
    Status = LsaLookupNames2(PolicyHandle, Count, 0, Names, ReferencedDomains, &Sids2);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Translate the returned structure */
    TranslatedSids = RtlAllocateHeap(RtlGetProcessHeap(), 0, Count * sizeof(LSA_TRANSLATED_SID));
    if (!TranslatedSids)
    {
        LsaFreeMemory(Sids2);
        return SCESTATUS_NOT_ENOUGH_RESOURCE;
    }
    RtlZeroMemory(Sids, Count * sizeof(PLSA_TRANSLATED_SID));
    for (i = 0; i < Count; i++)
    {
        TranslatedSids[i].Use = Sids2[i].Use;
        if (Sids2[i].Use != SidTypeInvalid && Sids2[i].Use != SidTypeUnknown)
        {
            TranslatedSids[i].DomainIndex = Sids2[i].DomainIndex;
            if (Sids2[i].Use != SidTypeDomain)
                TranslatedSids[i].RelativeId = *GetSidSubAuthority(Sids2[i].Sid, 0);
        }
    }
    LsaFreeMemory(Sids2);

    *Sids = TranslatedSids;

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
    return STATUS_NONE_MAPPED;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
LsaLookupSids(
    LSA_HANDLE PolicyHandle,
    ULONG Count,
    PSID *Sids,
    PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    PLSA_TRANSLATED_NAME *Names)
{
    static const UNICODE_STRING UserName = RTL_CONSTANT_STRING(L"Administrator");
    PLSA_REFERENCED_DOMAIN_LIST LocalDomains;
    PLSA_TRANSLATED_NAME LocalNames;

    TRACE("(%p,%u,%p,%p,%p) stub\n", PolicyHandle, Count, Sids,
          ReferencedDomains, Names);

    WARN("LsaLookupSids(): stub. Always returning 'Administrator'\n");
    if (Count != 1)
        return STATUS_NONE_MAPPED;
    LocalDomains = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(LSA_TRANSLATED_SID));
    if (!LocalDomains)
        return SCESTATUS_NOT_ENOUGH_RESOURCE;
    LocalNames = RtlAllocateHeap(RtlGetProcessHeap(), 0,  sizeof(LSA_TRANSLATED_NAME) + UserName.MaximumLength);
    if (!LocalNames)
    {
        LsaFreeMemory(LocalDomains);
        return SCESTATUS_NOT_ENOUGH_RESOURCE;
    }
    LocalDomains[0].Entries = 0;
    LocalDomains[0].Domains = NULL;
    LocalNames[0].Use = SidTypeWellKnownGroup;
    LocalNames[0].Name.Buffer = (LPWSTR)((ULONG_PTR)(LocalNames) + sizeof(LSA_TRANSLATED_NAME));
    LocalNames[0].Name.Length = UserName.Length;
    LocalNames[0].Name.MaximumLength = UserName.MaximumLength;
    RtlCopyMemory(LocalNames[0].Name.Buffer, UserName.Buffer, UserName.MaximumLength);

    *ReferencedDomains = LocalDomains;
    *Names = LocalNames;
    return STATUS_SUCCESS;
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
          SystemName?debugstr_w(SystemName->Buffer):"(null)",
          ObjectAttributes, DesiredAccess, PolicyHandle);

    RpcTryExcept
    {
        *PolicyHandle = NULL;

        Status = LsarOpenPolicy(SystemName->Buffer,
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
    return STATUS_NOT_IMPLEMENTED;
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
        case PolicyAccountDomainInformation: /* 5 */
        {
            struct di
            {
                POLICY_PRIMARY_DOMAIN_INFO ppdi;
                SID sid;
            };
            SID_IDENTIFIER_AUTHORITY localSidAuthority = {SECURITY_NT_AUTHORITY};

            struct di * xdi = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*xdi));
            HKEY key;
            BOOL useDefault = TRUE;
            LONG ret;

            if ((ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "System\\CurrentControlSet\\Services\\VxD\\VNETSUP", 0,
                    KEY_READ, &key)) == ERROR_SUCCESS)
            {
                DWORD size = 0;
                WCHAR wg[] = { 'W','o','r','k','g','r','o','u','p',0 };

                ret = RegQueryValueExW(key, wg, NULL, NULL, NULL, &size);
                if (ret == ERROR_MORE_DATA || ret == ERROR_SUCCESS)
                {
                    xdi->ppdi.Name.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                    HEAP_ZERO_MEMORY, size);
                    if ((ret = RegQueryValueExW(key, wg, NULL, NULL,
                        (LPBYTE)xdi->ppdi.Name.Buffer, &size)) == ERROR_SUCCESS)
                    {
                        xdi->ppdi.Name.Length = (USHORT)size;
                        useDefault = FALSE;
                    }
                    else
                    {
                        RtlFreeHeap(RtlGetProcessHeap(), 0, xdi->ppdi.Name.Buffer);
                        xdi->ppdi.Name.Buffer = NULL;
                    }
                }
                RegCloseKey(key);
            }
            if (useDefault)
                RtlCreateUnicodeStringFromAsciiz(&(xdi->ppdi.Name), "DOMAIN");
            TRACE("setting domain to \n");

            xdi->ppdi.Sid = &(xdi->sid);
            xdi->sid.Revision = SID_REVISION;
            xdi->sid.SubAuthorityCount = 1;
            xdi->sid.IdentifierAuthority = localSidAuthority;
            xdi->sid.SubAuthority[0] = SECURITY_LOCAL_SYSTEM_RID;
            *Buffer = xdi;
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
        case PolicyDnsDomainInformation:
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
    return STATUS_NOT_IMPLEMENTED;
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
