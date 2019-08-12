/*
 * Copyright 2011 Samuel Serapión
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

#include "ntlmssp.h"

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

BOOL
NtlmCompareCredentials(IN NTLMSSP_CREDENTIAL Credential1,
                       IN NTLMSSP_CREDENTIAL Credential2)
{
    UNIMPLEMENTED;
    return FALSE;
}

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
        NtlmFree(cred);
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

/* public functions */

SECURITY_STATUS
SEC_ENTRY
QueryCredentialsAttributesW(PCredHandle phCredential,
                            ULONG ulAttribute,
                            PVOID pBuffer)
{
    PNTLMSSP_CREDENTIAL credentials = NULL;
    PSecPkgContext_NamesW credname;
    SECURITY_STATUS ret;

    TRACE("(%p, %lx, %p)\n", phCredential, ulAttribute, pBuffer);

    credentials = NtlmReferenceCredential(phCredential->dwLower);

    switch(ulAttribute)
    {
    case SECPKG_ATTR_NAMES:
        credname = (PSecPkgContext_NamesW) pBuffer;
        credname->sUserName = _wcsdup((WCHAR*)credentials->UserNameW.Buffer);
        ret = SEC_E_OK;
        break;
    default:
        FIXME("QueryCredentialsAttributesW(%p, %lx, %p) Unimplemented\n",
            phCredential, ulAttribute, pBuffer);
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        break;
    }

    NtlmDereferenceCredential(phCredential->dwLower);
    return ret;
}

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

SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleW(IN OPTIONAL SEC_WCHAR *pszPrincipal,
                          IN OPTIONAL SEC_WCHAR *pszPackage,
                          IN ULONG fCredentialUse,
                          IN PLUID pLogonID,
                          IN PVOID pAuthData,
                          IN SEC_GET_KEY_FN pGetKeyFn,
                          IN PVOID pGetKeyArgument,
                          OUT PCredHandle phCredential,
                          OUT PTimeStamp ptsExpiry)
{

    PNTLMSSP_CREDENTIAL cred = NULL;
    PNTLMSSP_GLOBALS g;
    SECURITY_STATUS ret = SEC_E_OK;
    ULONG credFlags = fCredentialUse;
    EXT_STRING username, domain, password;
    BOOL foundCred = FALSE;
    LUID luidToUse = SYSTEM_LUID;

    TRACE("AcquireCredentialsHandleW(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);

    if (pGetKeyFn || pGetKeyArgument)
        WARN("msdn says these should always be null!\n");

    //initialize to null
    ExtWStrInit(&username, NULL);
    ExtWStrInit(&domain, NULL);
    ExtWStrInit(&password, NULL);

    if(fCredentialUse == SECPKG_CRED_OUTBOUND && pAuthData)
    {
        PSEC_WINNT_AUTH_IDENTITY_W auth_data = pAuthData;

        /* detect null session */
        if ((auth_data->User) && (auth_data->Password) &&
            (auth_data->Domain) && (!auth_data->UserLength) &&
            (!auth_data->PasswordLength) &&(!auth_data->DomainLength))
        {
            WARN("Using null session.\n");
            credFlags |= NTLM_CRED_NULLSESSION;
        }

        /* create unicode strings and null terminate buffers */

        if ((auth_data->User) &&
            (!ExtWStrSetN(&username, auth_data->User, auth_data->UserLength)))
        {
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto quit;
        }

        if ((auth_data->Password) &&
            (!ExtWStrSetN(&password, auth_data->Password, auth_data->PasswordLength)))
        {
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto quit;
        }

        if ((auth_data->Domain) &&
            (!ExtWStrSetN(&domain, auth_data->Domain, auth_data->DomainLength)))
        {
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto quit;
        }
    }

    /* FIXME: LOOKUP STORED CREDENTIALS!!! */

    /* we need to build a credential */
    if(!foundCred)
    {
        cred = (PNTLMSSP_CREDENTIAL)NtlmAllocate(sizeof(NTLMSSP_CREDENTIAL));
        cred->RefCount = 1;
        cred->ProcId = GetCurrentProcessId();//FIXME
        cred->UseFlags = credFlags;

        g = lockGlobals();
        cred->SecToken = g->NtlmSystemSecurityToken; //FIXME
        unlockGlobals(&g);

        /* FIX ME: check against LSA token */
        if((cred->SecToken == NULL) && !(credFlags & NTLM_CRED_NULLSESSION))
        {
            /* check privilages? */
            cred->LogonId = luidToUse;
        }

        if(domain.Buffer != NULL)
            cred->DomainNameW = domain;

        if(username.Buffer != NULL)
            cred->UserNameW = username;

        if(password.Buffer != NULL)
        {
            NtlmProtectMemory(password.Buffer, password.bUsed);
            cred->PasswordW = password;
        }

        EnterCriticalSection(&CredentialCritSect);
        InsertHeadList(&ValidCredentialList, &cred->Entry);
        LeaveCriticalSection(&CredentialCritSect);

        TRACE("added credential %x\n",cred);
        TRACE("%s %s %s\n", debugstr_w((WCHAR*)username.Buffer),
              debugstr_w((WCHAR*)password.Buffer),
              debugstr_w((WCHAR*)domain.Buffer));
    }

    /* return cred */
    phCredential->dwUpper = credFlags;
    phCredential->dwLower = (ULONG_PTR)cred;

    ptsExpiry->HighPart = 0x7FFFFF36;
    ptsExpiry->LowPart = 0xD5969FFF;


    /* free strings as we used recycled credentials */
    //if(foundCred)
quit:
    if (ret != SEC_E_OK)
    {
        ExtStrFree(&username);
        ExtStrFree(&domain);
        ExtStrFree(&password);
    }
    return ret;
}

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

SECURITY_STATUS
SEC_ENTRY
FreeCredentialsHandle(PCredHandle phCredential)
{
    TRACE("FreeCredentialsHandle %x %x\n", phCredential, phCredential->dwLower);

    if(!phCredential)
        return SEC_E_INVALID_HANDLE;

    NtlmDereferenceCredential(phCredential->dwLower);
    phCredential = NULL;

    return SEC_E_OK;
}
