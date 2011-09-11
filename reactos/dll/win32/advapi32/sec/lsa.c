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
WINE_DEFAULT_DEBUG_CHANNEL(advapi);


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
NTSTATUS
WINAPI
LsaClose(IN LSA_HANDLE ObjectHandle)
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
NTSTATUS
WINAPI
LsaDelete(IN LSA_HANDLE ObjectHandle)
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
 * @implemented
 */
NTSTATUS
WINAPI
LsaAddAccountRights(IN LSA_HANDLE PolicyHandle,
                    IN PSID AccountSid,
                    IN PLSA_UNICODE_STRING UserRights,
                    IN ULONG CountOfRights)
{
    LSAPR_USER_RIGHT_SET UserRightSet;
    NTSTATUS Status;

    TRACE("(%p,%p,%p,0x%08x) stub\n", PolicyHandle, AccountSid, UserRights, CountOfRights);

    UserRightSet.Entries = CountOfRights;
    UserRightSet.UserRights = (PRPC_UNICODE_STRING)UserRights;

    RpcTryExcept
    {
        Status = LsarAddAccountRights((LSAPR_HANDLE)PolicyHandle,
                                      (PRPC_SID)AccountSid,
                                      &UserRightSet);

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
LsaAddPrivilegesToAccount(IN LSA_HANDLE AccountHandle,
                          IN PPRIVILEGE_SET PrivilegeSet)
{
    NTSTATUS Status;

    TRACE("(%p,%p) stub\n", AccountHandle, PrivilegeSet);

    RpcTryExcept
    {
        Status = LsarAddPrivilegesToAccount((LSAPR_HANDLE)AccountHandle,
                                            (PLSAPR_PRIVILEGE_SET)PrivilegeSet);
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
LsaCreateAccount(IN LSA_HANDLE PolicyHandle,
                 IN PSID AccountSid,
                 IN ULONG Flags,
                 OUT PLSA_HANDLE AccountHandle)
{
    NTSTATUS Status;

    TRACE("(%p,%p,0x%08x,%p)\n", PolicyHandle, AccountSid, Flags, AccountHandle);

    RpcTryExcept
    {
        Status = LsarCreateAccount((LSAPR_HANDLE)PolicyHandle,
                                   AccountSid,
                                   Flags,
                                   AccountHandle);
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
LsaCreateTrustedDomain(IN LSA_HANDLE PolicyHandle,
                       IN PLSA_TRUST_INFORMATION TrustedDomainInformation,
                       IN ACCESS_MASK DesiredAccess,
                       OUT PLSA_HANDLE TrustedDomainHandle)
{
    NTSTATUS Status;

    TRACE("(%p,%p,0x%08x,%p)\n", PolicyHandle, TrustedDomainInformation,
          DesiredAccess, TrustedDomainHandle);

    RpcTryExcept
    {
        Status = LsarCreateTrustedDomain((LSAPR_HANDLE)PolicyHandle,
                                         (PLSAPR_TRUST_INFORMATION)TrustedDomainInformation,
                                         DesiredAccess,
                                         (PLSAPR_HANDLE)TrustedDomainHandle);
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
 * @implemented
 */
NTSTATUS
WINAPI
LsaDeleteTrustedDomain(IN LSA_HANDLE PolicyHandle,
                       IN PSID TrustedDomainSid)
{
    NTSTATUS Status;

    TRACE("(%p,%p)\n", PolicyHandle, TrustedDomainSid);

    RpcTryExcept
    {
        Status = LsarDeleteTrustedDomain((LSAPR_HANDLE)PolicyHandle,
                                         TrustedDomainSid);
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
LsaEnumerateAccountRights(IN LSA_HANDLE PolicyHandle,
                          IN PSID AccountSid,
                          OUT PLSA_UNICODE_STRING *UserRights,
                          OUT PULONG CountOfRights)
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
LsaFreeMemory(IN PVOID Buffer)
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
    LSAPR_TRANSLATED_SIDS TranslatedSids = {0, NULL};
    ULONG MappedCount = 0;
    NTSTATUS Status;

    TRACE("(%p,0x%08x,%p,%p,%p)\n", PolicyHandle, Count, Names,
          ReferencedDomains, Sids);

    if (ReferencedDomains == NULL || Sids == NULL)
        return STATUS_INVALID_PARAMETER;

    RpcTryExcept
    {
        *ReferencedDomains = NULL;
        *Sids = NULL;

        TranslatedSids.Entries = Count;

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
            MIDL_user_free(TranslatedSids.Sids);

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
LsaLookupNames2(IN LSA_HANDLE PolicyHandle,
                IN ULONG Flags,
                IN ULONG Count,
                IN PLSA_UNICODE_STRING Names,
                OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
                OUT PLSA_TRANSLATED_SID2 *Sids)
{
    LSAPR_TRANSLATED_SIDS_EX2 TranslatedSids = {0, NULL};
    ULONG MappedCount = 0;
    NTSTATUS Status;

    TRACE("(%p,0x%08x,0x%08x,%p,%p,%p) stub\n", PolicyHandle, Flags,
          Count, Names, ReferencedDomains, Sids);

    if (ReferencedDomains == NULL || Sids == NULL)
        return STATUS_INVALID_PARAMETER;

    RpcTryExcept
    {
        *ReferencedDomains = NULL;
        *Sids = NULL;

        TranslatedSids.Entries = Count;

        Status = LsarLookupNames3((LSAPR_HANDLE)PolicyHandle,
                                  Count,
                                  (PRPC_UNICODE_STRING)Names,
                                  (PLSAPR_REFERENCED_DOMAIN_LIST *)ReferencedDomains,
                                  &TranslatedSids,
                                  LsapLookupWksta,
                                  &MappedCount,
                                  Flags,
                                  2);

        *Sids = (PLSA_TRANSLATED_SID2)TranslatedSids.Sids;
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        if (TranslatedSids.Sids != NULL)
            MIDL_user_free(TranslatedSids.Sids);

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
ULONG
WINAPI
LsaNtStatusToWinError(IN NTSTATUS Status)
{
    TRACE("(%lx)\n", Status);
    return RtlNtStatusToDosError(Status);
}


/*
 * @implemented
 */
NTSTATUS
WINAPI
LsaOpenAccount(IN LSA_HANDLE PolicyHandle,
               IN PSID AccountSid,
               IN ULONG Flags,
               OUT PLSA_HANDLE AccountHandle)
{
    NTSTATUS Status;

    TRACE("(%p,%p,0x%08x,%p)\n", PolicyHandle, AccountSid, Flags, AccountHandle);

    RpcTryExcept
    {
        Status = LsarOpenAccount((LSAPR_HANDLE)PolicyHandle,
                                 AccountSid,
                                 Flags,
                                 AccountHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return Status;
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
 * @implemented
 */
NTSTATUS
WINAPI
LsaOpenPolicy(IN PLSA_UNICODE_STRING SystemName,
              IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
              IN ACCESS_MASK DesiredAccess,
              OUT PLSA_HANDLE PolicyHandle)
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
 * @implemented
 */
NTSTATUS
WINAPI
LsaOpenTrustedDomainByName(IN LSA_HANDLE PolicyHandle,
                           IN PLSA_UNICODE_STRING TrustedDomainName,
                           IN ACCESS_MASK DesiredAccess,
                           OUT PLSA_HANDLE TrustedDomainHandle)
{
    NTSTATUS Status;

    TRACE("(%p,%p,0x%08x,%p)\n", PolicyHandle, TrustedDomainName,
          DesiredAccess, TrustedDomainHandle);

    RpcTryExcept
    {
        Status = LsarOpenTrustedDomainByName((LSAPR_HANDLE)PolicyHandle,
                                             (PRPC_UNICODE_STRING)TrustedDomainName,
                                             DesiredAccess,
                                             TrustedDomainHandle);
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
 * @implemented
 */
NTSTATUS
WINAPI
LsaQueryInformationPolicy(IN LSA_HANDLE PolicyHandle,
                          IN POLICY_INFORMATION_CLASS InformationClass,
                          OUT PVOID *Buffer)
{
    PLSAPR_POLICY_INFORMATION PolicyInformation = NULL;
    NTSTATUS Status;

    TRACE("(%p,0x%08x,%p)\n", PolicyHandle, InformationClass, Buffer);

    RpcTryExcept
    {
        Status = LsarQueryInformationPolicy((LSAPR_HANDLE)PolicyHandle,
                                            InformationClass,
                                            &PolicyInformation);
        *Buffer = PolicyInformation;
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        if (PolicyInformation != NULL)
            MIDL_user_free(PolicyInformation);

        Status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    TRACE("Done (Status: 0x%08x)\n", Status);

    return Status;
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
 * @implemented
 */
NTSTATUS
WINAPI
LsaQueryTrustedDomainInfoByName(IN LSA_HANDLE PolicyHandle,
                                IN PLSA_UNICODE_STRING TrustedDomainName,
                                IN TRUSTED_INFORMATION_CLASS InformationClass,
                                OUT PVOID *Buffer)
{
    NTSTATUS Status;

    TRACE("(%p,%p,%d,%p)\n", PolicyHandle, TrustedDomainName, InformationClass, Buffer);

    if (InformationClass == TrustedDomainAuthInformationInternal ||
        InformationClass == TrustedDomainFullInformationInternal)
        return STATUS_INVALID_INFO_CLASS;

    RpcTryExcept
    {
        Status = LsarQueryTrustedDomainInfoByName((LSAPR_HANDLE)PolicyHandle,
                                                  (PRPC_UNICODE_STRING)TrustedDomainName,
                                                  InformationClass,
                                                  (unsigned long *)Buffer); // Shuld be: (PLSAPR_POLICY_INFORMATION *)Buffer
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
LsaRemoveAccountRights(IN LSA_HANDLE PolicyHandle,
                       IN PSID AccountSid,
                       IN BOOLEAN AllRights,
                       IN PLSA_UNICODE_STRING UserRights,
                       IN ULONG CountOfRights)
{
    LSAPR_USER_RIGHT_SET UserRightSet;

    TRACE("(%p,%p,%d,%p,0x%08x) stub\n", PolicyHandle, AccountSid, AllRights, UserRights, CountOfRights);

    UserRightSet.Entries = CountOfRights;
    UserRightSet.UserRights = (PRPC_UNICODE_STRING)UserRights;

    RpcTryExcept
    {
        LsarRemoveAccountRights((LSAPR_HANDLE)PolicyHandle,
                                (PRPC_SID)AccountSid,
                                AllRights,
                                &UserRightSet);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

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
 * @implemented
 */
NTSTATUS
WINAPI
LsaSetInformationPolicy(IN LSA_HANDLE PolicyHandle,
                        IN POLICY_INFORMATION_CLASS InformationClass,
                        IN PVOID Buffer)
{
    NTSTATUS Status;

    TRACE("(%p,0x%08x,%p)\n", PolicyHandle, InformationClass, Buffer);

    RpcTryExcept
    {
        Status = LsarSetInformationPolicy((LSAPR_HANDLE)PolicyHandle,
                                          InformationClass,
                                          (PLSAPR_POLICY_INFORMATION)Buffer);
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
