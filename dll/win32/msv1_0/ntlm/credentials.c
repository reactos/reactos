/*
 * PROJECT:     ReactOS ntlm implementation (msv1_0)
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ntlm credentials (header)
 * COPYRIGHT:   Copyright 2011 Samuel Serapión
 *              Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#include "../precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

CRITICAL_SECTION CredentialCritSect;
LIST_ENTRY ValidCredentialList;

/* private functions */
NTSTATUS
NtlmCredentialInitialize(VOID)
{
    InitializeCriticalSection(&CredentialCritSect);
    InitializeListHead(&ValidCredentialList);
    return STATUS_SUCCESS;
}

#ifdef __UNUSED__
BOOL
NtlmCompareCredentials(IN NTLMSSP_CREDENTIAL Credential1,
                       IN NTLMSSP_CREDENTIAL Credential2)
{
    UNIMPLEMENTED;
    return FALSE;
}
#endif

PNTLMSSP_CREDENTIAL
NtlmReferenceCredential(IN ULONG_PTR Handle)
{
    PNTLMSSP_CREDENTIAL cred;
    EnterCriticalSection(&CredentialCritSect);

    cred = (PNTLMSSP_CREDENTIAL)Handle;

    /* sanity */
    ASSERT(cred);
    TRACE("%p refcount %lx\n",cred, cred->RefCount);
    ASSERT(cred->RefCount > 0);

    /* reference */
    cred->RefCount++;

    LeaveCriticalSection(&CredentialCritSect);
    return cred;
}

VOID
NtlmDereferenceCredential(IN ULONG_PTR Handle)
{
    PNTLMSSP_CREDENTIAL cred;
    EnterCriticalSection(&CredentialCritSect);

    cred = (PNTLMSSP_CREDENTIAL)Handle;

    /* sanity */
    ASSERT(cred);
    TRACE("%p refcount %lx\n",cred, cred->RefCount);
    ASSERT(cred->RefCount >= 1);

    /* decrement reference */
    cred->RefCount--;

    /* check for object rundown */
    if (cred->RefCount == 0)
    {
        TRACE("Deleting credential %p\n",cred);

        /* free memory */
        ExtStrFree(&cred->DomainNameW);
        ExtStrFree(&cred->UserNameW);
        ExtStrFree(&cred->PasswordW);
        if (cred->SecToken)
        {
            PNTLMSSP_GLOBALS g = lockGlobals();
            /* Do not close globals! FIXME */
            if (cred->SecToken != g->NtlmSystemSecurityToken)
                NtClose(cred->SecToken);
            unlockGlobals(&g);
        }

        /* remove from list */
        RemoveEntryList(&cred->Entry);

        /* delete object */
        NtlmFree(cred, FALSE);
    }
    LeaveCriticalSection(&CredentialCritSect);
}

VOID
NtlmCredentialTerminate(VOID)
{
    EnterCriticalSection(&CredentialCritSect);

    /* dereference all items */
    while (!IsListEmpty(&ValidCredentialList))
    {
        PNTLMSSP_CREDENTIAL Credential;
        Credential = CONTAINING_RECORD(ValidCredentialList.Flink,
                                       NTLMSSP_CREDENTIAL,
                                       Entry);

        NtlmDereferenceCredential((ULONG_PTR)Credential);
    }

    LeaveCriticalSection(&CredentialCritSect);

    /* free critical section */
    DeleteCriticalSection(&CredentialCritSect);

    return;
}

SECURITY_STATUS
SEC_ENTRY
NtlmQueryCredentialsAttributes_Names(
    IN PNTLMSSP_CREDENTIAL Cred,
    IN ULONG ulAttribute,
    IN OUT PVOID pBuffer)
{
    SECURITY_STATUS ret;
    PVOID LsaClientUserName = NULL;
    SEC_WCHAR *LocalUserName = NULL;
    ULONG BytesNeeded;
    SecPkgContext_NamesW CredNamesW;
    // we return unicode ... for the return value
    // i'm not sure how to distinguish between
    // unicode and ansi.
    BytesNeeded = Cred->UserNameW.bUsed +
                  sizeof(WCHAR);

    LocalUserName = NtlmAllocate(BytesNeeded, TRUE);
    if (Cred->UserNameW.bUsed > 0)
        wcsncpy(LocalUserName,
                (WCHAR*)Cred->UserNameW.Buffer,
                BytesNeeded / sizeof(WCHAR));

    // lsa mode: copy data to client
    if (inUserMode)
    {
        CredNamesW.sUserName = LocalUserName;
        memcpy(pBuffer, &CredNamesW, sizeof(SecPkgContext_NamesW));
        ret = SEC_E_OK;
        goto done;
    }

    if (inLsaMode)
    {
        NTSTATUS status;

        // allocate client buffer for username
        status = LsaFunctions->AllocateClientBuffer(NULL, BytesNeeded,
                                                    &LsaClientUserName);
        if (!NT_SUCCESS(status))
        {
            ret = status;
            goto done;
        }

        // copy username to clientbuffer
        status = LsaFunctions->CopyToClientBuffer(NULL, BytesNeeded,
                                                  LsaClientUserName,
                                                  LocalUserName);
        if (!NT_SUCCESS(status))
        {
            TRACE("CopyToClientBuffer failed!\n");
            ret = status;
            goto done;
        }

        // copy SecPkgContext_NamesW (struct) to client buffer
        CredNamesW.sUserName = LsaClientUserName;
        status = LsaFunctions->CopyToClientBuffer(NULL, sizeof(SecPkgContext_NamesW),
                                                  pBuffer, &CredNamesW);
        if (!NT_SUCCESS(status))
        {
            TRACE("CopyToClientBuffer failed!\n");
            ret = status;
            goto done;
        }

        ret = SEC_E_OK;
        goto done;
    }

    ERR("unknown mode!");
    ret = SEC_E_INTERNAL_ERROR;

done:
    if (inLsaMode)
    {
        if (ret != SEC_E_OK)
        {
            if (LsaClientUserName != NULL)
                LsaFunctions->FreeClientBuffer(NULL, LsaClientUserName);
        }
        // LocalUserName was only temporary in lsa mode
        if (LocalUserName != NULL)
            NtlmFree(LocalUserName, TRUE);
    }

    return ret;
}

SECURITY_STATUS
SEC_ENTRY
NtlmQueryCredentialsAttributes(
    IN LSA_SEC_HANDLE hCredential,
    IN ULONG ulAttribute,
    IN OUT PVOID pBuffer)
{
    PNTLMSSP_CREDENTIAL credentials = NULL;
    SECURITY_STATUS ret;

    TRACE("(%p, %lx, %p)\n", hCredential, ulAttribute, pBuffer);

    if (hCredential == 0)
    {
        ERR("NtlmQueryCredentialsAttributes called with hCredential = 0\n");
        return SEC_E_INVALID_HANDLE;
    }

    credentials = NtlmReferenceCredential(hCredential);

    switch (ulAttribute)
    {
        case SECPKG_CRED_ATTR_NAMES:
        {
            ret = NtlmQueryCredentialsAttributes_Names(credentials, ulAttribute, pBuffer);
            break;
        }
        default:
        {
            FIXME("QueryCredentialsAttributesW(%p, %lx, %p) Unimplemented\n",
                hCredential, ulAttribute, pBuffer);
            ret = SEC_E_UNSUPPORTED_FUNCTION;
            break;
        }
    }

    return ret;
}

#ifdef __UNUSED__
SECURITY_STATUS
SEC_ENTRY
QueryCredentialsAttributesA(IN PCredHandle phCredential,
                            IN ULONG ulAttribute,
                            OUT PVOID pBuffer)
{
    //PNTLMSSP_CREDENTIAL credentials = NULL;
    SECURITY_STATUS ret;

    TRACE("(%p, %lx, %p)\n", phCredential, ulAttribute, pBuffer);

    //credentials = NtlmReferenceCredential(phCredential->dwLower);

    switch(ulAttribute)
    {
    default:
        FIXME("QueryCredentialsAttributesA(%p, %lx, %p) Unimplemented\n",
            phCredential, ulAttribute, pBuffer);
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        break;
    }

    //NtlmDereferenceCredential(phCredential->dwLower);
    return ret;
}
#endif

/**
 * @brief lookup credentials
 *
 * @param LogonId
 * @param CredentialsUseFlags
 * @param Unknown1
 * @param S1
 * @param Password
 * @return
 */
BOOLEAN
LookupCrdentialsHandle(
    _In_ PLUID LogonId,
    _In_ ULONG CredentialsUseFlags,
    _In_ DWORD ProcessId,
    _In_ PEXT_STRING_W UserName, /* domain or name */
    _In_ PEXT_STRING_W Password,
    //_In_ BOOLEAN Impersonating,
    _Out_ PNTLMSSP_CREDENTIAL *FoundCred)
{
    PLIST_ENTRY CredEntry;
    PNTLMSSP_CREDENTIAL Cred;
    *FoundCred = NULL;

    EnterCriticalSection(&CredentialCritSect);
    for (CredEntry = ValidCredentialList.Flink;
         CredEntry != &ValidCredentialList;
         CredEntry = CredEntry->Flink)
    {
        Cred = CONTAINING_RECORD(CredEntry, NTLMSSP_CREDENTIAL, Entry);
        //FIXME: What should be equal?
        //FIXME: compare password??
        //FIXME: Check expirationtime ...
        if ((Cred->LogonId.LowPart != LogonId->LowPart) ||
            (Cred->LogonId.HighPart != LogonId->HighPart) ||
            (Cred->UseFlags != CredentialsUseFlags) ||
            (Cred->ProcId != ProcessId) ||
            (!ExtWStrIsEqual1(&Cred->UserNameW, UserName, FALSE)))
            //(!ExtWStrIsEqual1(&Cred->DomainNameW, DomainName, FALSE))
            continue;
        // we found it.
        *FoundCred = Cred;
        break;
    }
    LeaveCriticalSection(&CredentialCritSect);

    return (*FoundCred != NULL);
}

NTSTATUS
IntAcquireCredentialsHandle(
    IN PLUID LogonId,
    IN PSECPKG_CLIENT_INFO ClientInfo,
    IN ULONG CredentialsUseFlags,
    IN PEXT_STRING_W UserName,
    IN PEXT_STRING_W Domain,
    IN PEXT_STRING_W Password,
    OUT PNTLMSSP_CREDENTIAL *Cred,
    OUT PTimeStamp ExpirationTime)
{
    PNTLMSSP_GLOBALS g;
    PNTLMSSP_CREDENTIAL NewCred;

    if (CredentialsUseFlags != SECPKG_CRED_OUTBOUND)
    {
        // FIXME: todo check machine login
    }

    if (LookupCrdentialsHandle(LogonId, CredentialsUseFlags,
                               ClientInfo->ProcessID, UserName,
                               Password, Cred))
        return STATUS_SUCCESS;

    // we need to build a credential
    NewCred = (PNTLMSSP_CREDENTIAL)NtlmAllocate(sizeof(NTLMSSP_CREDENTIAL), FALSE);
    NewCred->RefCount = 1;
    NewCred->LogonId = *LogonId;
    NewCred->ProcId = ClientInfo->ProcessID;
    NewCred->UseFlags = CredentialsUseFlags;
    // FIXME: What is a suitable value for ExpirationTime
    NewCred->ExpirationTime.HighPart = 0x7FFFFF36;
    NewCred->ExpirationTime.LowPart = 0xD5969FFF;

    g = lockGlobals();
    NewCred->SecToken = g->NtlmSystemSecurityToken; //FIXME
    unlockGlobals(&g);

    /* NEEDED??
     *  FIX ME: check against LSA token * /
    if ((cred->SecToken == NULL) && !(CredentialsUseFlags & NTLM_CRED_NULLSESSION))
    {
        / * check privilages? * /
        cred->LogonId = luidToUse;
    } */

    if (Domain->Buffer != NULL)
    {
        NewCred->DomainNameW = *Domain;
        // prevent freeing from caller
        ExtWStrInit(Domain, NULL);
    }

    if (UserName->Buffer != NULL)
    {
        NewCred->UserNameW = *UserName;
        // prevent freeing from caller
        ExtWStrInit(UserName, NULL);
    }

    if (Password->Buffer != NULL)
    {
        NewCred->PasswordW = *Password;
        // prevent freeing from caller
        ExtWStrInit(Password, NULL);
        NtlmProtectMemory(NewCred->PasswordW.Buffer, NewCred->PasswordW.bUsed);
    }

    EnterCriticalSection(&CredentialCritSect);
    InsertHeadList(&ValidCredentialList, &NewCred->Entry);
    LeaveCriticalSection(&CredentialCritSect);

    TRACE("added credential %x\n", NewCred);
    TRACE("%s %s %s\n", debugstr_w((WCHAR*)UserName->Buffer),
          debugstr_w((WCHAR*)Password->Buffer),
          debugstr_w((WCHAR*)Domain->Buffer));

    // return cred
    *Cred = NewCred;
    *ExpirationTime = NewCred->ExpirationTime;

    return STATUS_SUCCESS;
}

SECURITY_STATUS
IntAcquireCredWithAuthData(
    IN PLUID LogonId,
    IN PSECPKG_CLIENT_INFO ClientInfo,
    IN PVOID pAuthData,
    IN ULONG CredentialsUseFlags,
    OUT PNTLMSSP_CREDENTIAL *NewCred,
    OUT PTimeStamp ptsExpiry)
{
    NTSTATUS Status;
    SECURITY_STATUS ret;
    SECPKG_CALL_INFO CallInfo;
    PSEC_WINNT_AUTH_IDENTITY_W AuthData;
    ULONG UserNameByteLen;
    ULONG PasswordByteLen;
    ULONG DomainByteLen;
    PVOID LocalBuffer = NULL;
    UNICODE_STRING UserName;
    UNICODE_STRING Password;
    UNICODE_STRING Domain;
    BOOLEAN IsAnsiCall;
    // temp for conv -> EXT_STRING_W
    EXT_STRING_W _UserName;
    EXT_STRING_W _Password;
    EXT_STRING_W _Domain;

    RtlInitUnicodeString(&UserName, NULL);
    RtlInitUnicodeString(&Password, NULL);
    RtlInitUnicodeString(&Domain, NULL);
    ExtWStrInit(&_UserName, NULL);
    ExtWStrInit(&_Password, NULL);
    ExtWStrInit(&_Domain, NULL);

    Status = LsaFunctions->GetCallInfo(&CallInfo);
    if (!NT_SUCCESS(Status))
    {
        ret = SEC_E_INTERNAL_ERROR;
        goto done;
    }

    IsAnsiCall = ((CallInfo.Attributes & SECPKG_CALL_ANSI) != 0);
    if (IsAnsiCall)
    {
        WARN("ANSI call not supported!\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        goto done;
    }

    if (inLsaMode)
    {
        ULONG AuthDataSize = sizeof(SEC_WINNT_AUTH_IDENTITY_W);
        ASSERT(sizeof(SEC_WINNT_AUTH_IDENTITY_W) == sizeof(SEC_WINNT_AUTH_IDENTITY_A));

        LocalBuffer = NtlmAllocate(AuthDataSize, TRUE);
        Status = LsaFunctions->CopyFromClientBuffer(NULL, AuthDataSize,
                                                    LocalBuffer, pAuthData);
        if (!NT_SUCCESS(Status))
        {
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }

        AuthData = (PSEC_WINNT_AUTH_IDENTITY_W)LocalBuffer;
    }
    else if (inUserMode)
    {
        AuthData = (PSEC_WINNT_AUTH_IDENTITY_W)pAuthData;
    }
    else
    {
        ERR("Unknown mode (lsa/user)!");
        ret = SEC_E_INTERNAL_ERROR;
        goto done;
    }

    // calculate string sizes and allocate memory
    UserNameByteLen = AuthData->UserLength * sizeof(WCHAR);
    PasswordByteLen = AuthData->PasswordLength * sizeof(WCHAR);
    DomainByteLen = AuthData->DomainLength * sizeof(WCHAR);

    if (!NtlmUnicodeStringAlloc(&UserName, UserNameByteLen + sizeof(WCHAR)) ||
        !NtlmUnicodeStringAlloc(&Password, PasswordByteLen + sizeof(WCHAR)) ||
        !NtlmUnicodeStringAlloc(&Domain, DomainByteLen + sizeof(WCHAR)))
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto done;
    }

    // copy data from client buffer
    if (inLsaMode)
    {
        Status = LsaFunctions->CopyFromClientBuffer(NULL, UserNameByteLen,
                                                    UserName.Buffer, AuthData->User);
        if (!NT_SUCCESS(Status))
        {
            ERR("failed to allocate memory!");
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
        UserName.Length = UserNameByteLen;

        Status = LsaFunctions->CopyFromClientBuffer(NULL, PasswordByteLen,
                                                    Password.Buffer, AuthData->Password);
        if (!NT_SUCCESS(Status))
        {
            ERR("failed to allocate memory!");
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
        Password.Length = PasswordByteLen;

        Status = LsaFunctions->CopyFromClientBuffer(NULL, DomainByteLen,
                                                    Domain.Buffer, AuthData->Domain);
        if (!NT_SUCCESS(Status))
        {
            ERR("failed to allocate memory!");
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
        Domain.Length = DomainByteLen;
    }
    else
    {
        if (!NtlmUnicodeStringAllocAndCopyW(&UserName, AuthData->User, UserNameByteLen) ||
            !NtlmUnicodeStringAllocAndCopyW(&Password, AuthData->Password, PasswordByteLen) ||
            !NtlmUnicodeStringAllocAndCopyW(&Domain, AuthData->Domain, DomainByteLen))
        {
            ERR("failed to allocate memory!");
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto done;
        }
    }
    

    // detect null session
    if ((UserName.Length == 0) &&
        (Password.Length == 0) &&
        (Domain.Length == 0))
    {
        WARN("Using null session.\n");
        CredentialsUseFlags |= NTLM_CRED_NULLSESSION;
    }

    NtlmInitExtStrWFromUnicodeString(&_UserName, &UserName);
    NtlmInitExtStrWFromUnicodeString(&_Password, &Password);
    NtlmInitExtStrWFromUnicodeString(&_Domain, &Domain);
    ret = IntAcquireCredentialsHandle(LogonId, ClientInfo,
                                      CredentialsUseFlags,
                                      &_UserName, &_Password, &_Domain,
                                      NewCred, ptsExpiry);
done:
    if (LocalBuffer)
        NtlmFree(LocalBuffer, TRUE);
    NtlmUnicodeStringFree(&UserName);
    NtlmUnicodeStringFree(&Password);
    NtlmUnicodeStringFree(&Domain);
    ExtStrFree(&_UserName);
    ExtStrFree(&_Password);
    ExtStrFree(&_Domain);
    return ret;
}

/**
 * @brief LSA mode only!
 * @param OPTIONAL
 * @param pszPrincipal
 * @param OPTIONAL
 * @param pszPackage
 * @param ULONG
 * @param PLUID
 * @param PVOID
 * @param SEC_GET_KEY_FN
 * @param PVOID
 * @param PCredHandle
 * @param PTimeStamp
 */
SECURITY_STATUS
SEC_ENTRY
NtlmAcquireCredentialsHandle(
    IN OPTIONAL SEC_WCHAR *pszPrincipal,
    IN OPTIONAL SEC_WCHAR *pszPackage,
    IN ULONG fCredentialUse,
    IN PLUID pLogonID,
    IN PVOID pAuthData,
    IN SEC_GET_KEY_FN pGetKeyFn,
    IN PVOID pGetKeyArgument,
    OUT PLSA_SEC_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry)
{
    NTSTATUS status;
    SECURITY_STATUS ret = SEC_E_OK;
    EXT_STRING username, domain, password;
    //LUID luidToUse = SYSTEM_LUID;
    PNTLMSSP_CREDENTIAL Cred;

    TRACE("AcquireCredentialsHandleW(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);

    if (pGetKeyFn || pGetKeyArgument)
        WARN("msdn says these should always be null!\n");

    /* initialize to null */
    ExtWStrInit(&username, NULL);
    ExtWStrInit(&domain, NULL);
    ExtWStrInit(&password, NULL);

    if ((fCredentialUse & SECPKG_CRED_BOTH) != 0)
    {
        SECPKG_CLIENT_INFO ClientInfo;

        status = LsaFunctions->GetClientInfo(&ClientInfo);
        if (!NT_SUCCESS(status))
        {
            ERR("LsaFunctions->GetClientInfo failed!\n");
            ret = SEC_E_INTERNAL_ERROR;
            goto quit;
        }

        if (pLogonID != NULL)
        {
            if ((pLogonID->LowPart != 0) ||
                (pLogonID->HighPart != 0))
            {
                //...
                ERR("FIMXE -> We have a Logon Id!\n");
            }
            // label: CmpAuthData
            else if (pAuthData == NULL)
            {
                // compare ucs
                if (FALSE)//pwd != NULL)
                {
                    /*res = NtLmDuplicatePassword(dst, src);
                    if (NT_SUCCESS(res))
                        goto quit;*/
                    // duplicate pwd NtLmDuplicatePassword
                    // check error
                } else
                {

                }
                ret = IntAcquireCredentialsHandle(
                    pLogonID, &ClientInfo, fCredentialUse,
                    &username, &domain, &password, &Cred, ptsExpiry);
                if (NT_SUCCESS(ret))
                {
                    // we're done
                    *phCredential = (LSA_SEC_HANDLE)Cred;
                    goto quit;
                }
            } else
            {
                ret = IntAcquireCredWithAuthData(
                    pLogonID, &ClientInfo, pAuthData,
                    fCredentialUse, &Cred, ptsExpiry);
                if (NT_SUCCESS(ret))
                {
                    // we're done
                    *phCredential = (LSA_SEC_HANDLE)Cred;
                    goto quit;
                }
            }
        }
        else
        {
            // TODO LogonId = NULL ->
            // use ClientInfo.LogonId + Test
        }


        /*
        {
            PWKSTA_USER_INFO_1 pUsrInfo1 = NULL;
            SECPKG_CLIENT_INFO ClientInfo;

            if (NetWkstaUserGetInfo(NULL, 1, (LPBYTE*)&pUsrInfo1) != NERR_Success)
            {
                ERR("NetWkstaUserGetInfo failed.\n");
                ret = SEC_E_INTERNAL_ERROR;
                goto quit;
            }
            if (!ExtWStrSet(&username, pUsrInfo1->wkui1_username) ||
                !ExtWStrSet(&domain, pUsrInfo1->wkui1_logon_domain) ||
                !ExtWStrSet(&password, L""))
            {
                NetApiBufferFree(pUsrInfo1);
                ERR("ExtWStrSet failed\n");
                ret = SEC_E_INSUFFICIENT_MEMORY;
                goto quit;
            }
            NetApiBufferFree(pUsrInfo1);
        }*/
    }

    TRACE("returning ...\n");

    /* free strings as we used recycled credentials */
    //if(foundCred)
quit:
    ExtStrFree(&username);
    ExtStrFree(&domain);
    ExtStrFree(&password);
    return ret;
}

#ifdef __NOTUSED__
SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleA(SEC_CHAR *pszPrincipal,
                          SEC_CHAR *pszPackage,
                          ULONG fCredentialUse,
                          PLUID pLogonID,
                          PVOID pAuthData,
                          SEC_GET_KEY_FN pGetKeyFn,
                          PVOID pGetKeyArgument,
                          PCredHandle phCredential,
                          PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_OK;
    int user_sizeW, domain_sizeW, passwd_sizeW;
    
    SEC_WCHAR *user = NULL, *domain = NULL, *passwd = NULL, *package = NULL;
    
    PSEC_WINNT_AUTH_IDENTITY_W pAuthDataW = NULL;
    PSEC_WINNT_AUTH_IDENTITY_A identity  = NULL;

    TRACE("AcquireCredentialsHandleA(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_a(pszPrincipal), debugstr_a(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);
    
    if(pszPackage != NULL)
    {
        int package_sizeW = MultiByteToWideChar(CP_ACP, 0, pszPackage, -1,
                NULL, 0);

        package = HeapAlloc(GetProcessHeap(), 0, package_sizeW * 
                sizeof(SEC_WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pszPackage, -1, package, package_sizeW);
    }

    if(pAuthData != NULL)
    {
        identity = pAuthData;

        if(identity->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
        {
            pAuthDataW = HeapAlloc(GetProcessHeap(), 0, 
                    sizeof(SEC_WINNT_AUTH_IDENTITY_W));

            if(identity->UserLength != 0)
            {
                user_sizeW = MultiByteToWideChar(CP_ACP, 0, 
                    (LPCSTR)identity->User, identity->UserLength, NULL, 0);
                user = HeapAlloc(GetProcessHeap(), 0, user_sizeW * 
                        sizeof(SEC_WCHAR));
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)identity->User, 
                    identity->UserLength, user, user_sizeW);
            }
            else
            {
                user_sizeW = 0;
            }
             
            if(identity->DomainLength != 0)
            {
                domain_sizeW = MultiByteToWideChar(CP_ACP, 0, 
                    (LPCSTR)identity->Domain, identity->DomainLength, NULL, 0);
                domain = HeapAlloc(GetProcessHeap(), 0, domain_sizeW 
                    * sizeof(SEC_WCHAR));
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)identity->Domain, 
                    identity->DomainLength, domain, domain_sizeW);
            }
            else
            {
                domain_sizeW = 0;
            }

            if(identity->PasswordLength != 0)
            {
                passwd_sizeW = MultiByteToWideChar(CP_ACP, 0, 
                    (LPCSTR)identity->Password, identity->PasswordLength,
                    NULL, 0);
                passwd = HeapAlloc(GetProcessHeap(), 0, passwd_sizeW
                    * sizeof(SEC_WCHAR));
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)identity->Password,
                    identity->PasswordLength, passwd, passwd_sizeW);
            }
            else
            {
                passwd_sizeW = 0;
            }
            
            pAuthDataW->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            pAuthDataW->User = user;
            pAuthDataW->UserLength = user_sizeW;
            pAuthDataW->Domain = domain;
            pAuthDataW->DomainLength = domain_sizeW;
            pAuthDataW->Password = passwd;
            pAuthDataW->PasswordLength = passwd_sizeW;
        }
        else
        {
            pAuthDataW = (PSEC_WINNT_AUTH_IDENTITY_W)identity;
        }
    }       
    
    ret = AcquireCredentialsHandleW(NULL, package, fCredentialUse, 
            pLogonID, pAuthDataW, pGetKeyFn, pGetKeyArgument, phCredential,
            ptsExpiry);
    
    HeapFree(GetProcessHeap(), 0, package);
    HeapFree(GetProcessHeap(), 0, user);
    HeapFree(GetProcessHeap(), 0, domain);
    HeapFree(GetProcessHeap(), 0, passwd);
    if(pAuthDataW != (PSEC_WINNT_AUTH_IDENTITY_W)identity)
        HeapFree(GetProcessHeap(), 0, pAuthDataW);
    
    return ret;
}
#endif

SECURITY_STATUS
SEC_ENTRY
NtlmFreeCredentialsHandle(
    LSA_SEC_HANDLE hCredential)
{
    TRACE("FreeCredentialsHandle %x\n", hCredential);

    if(hCredential == 0)
        return SEC_E_INVALID_HANDLE;

    NtlmDereferenceCredential(hCredential);

    return SEC_E_OK;
}
