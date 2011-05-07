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

#include "ntlm.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/***********************************************************************
 *              QueryCredentialsAttributesW
 */
SECURITY_STATUS SEC_ENTRY QueryCredentialsAttributesW(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %d, %p)\n", phCredential, ulAttribute, pBuffer);

    if(ulAttribute == SECPKG_ATTR_NAMES)
    {
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    
    return ret;
}


/***********************************************************************
 *              QueryCredentialsAttributesA
 */
SECURITY_STATUS SEC_ENTRY QueryCredentialsAttributesA(
        PCredHandle phCredential, ULONG ulAttribute, PVOID pBuffer)
{
    SECURITY_STATUS ret;

    TRACE("(%p, %d, %p)\n", phCredential, ulAttribute, pBuffer);

    if(ulAttribute == SECPKG_ATTR_NAMES)
    {
        FIXME("SECPKG_CRED_ATTR_NAMES: stub\n");
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    
    return ret;
}

/***********************************************************************
 *              AcquireCredentialsHandleW
 */
SECURITY_STATUS SEC_ENTRY AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_UNSUPPORTED_FUNCTION;
    PNtlmCredentials cred = NULL;
    SEC_WCHAR *username = NULL, *domain = NULL;

    ERR("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);

    FIXME("AcquireCredentialsHandleW Unimplemented\n");
    switch(fCredentialUse)
    {
        case SECPKG_CRED_INBOUND:
            cred = HeapAlloc(GetProcessHeap(), 0, sizeof(*cred));
            if (!cred)
                ret = SEC_E_INSUFFICIENT_MEMORY;
            else
            {
                cred->mode = NTLM_SERVER;
                cred->username_arg = NULL;
                cred->domain_arg = NULL;
                cred->password = NULL;
                cred->pwlen = 0;
                phCredential->dwUpper = fCredentialUse;
                phCredential->dwLower = (ULONG_PTR)cred;
                ret = SEC_E_OK;
            }
            break;
        case SECPKG_CRED_OUTBOUND:
            {
                cred = HeapAlloc(GetProcessHeap(), 0, sizeof(*cred));
                if (!cred)
                {
                    ret = SEC_E_INSUFFICIENT_MEMORY;
                    break;
                }
                cred->mode = NTLM_CLIENT;
                cred->username_arg = NULL;
                cred->domain_arg = NULL;
                cred->password = NULL;
                cred->pwlen = 0;

                if(pAuthData != NULL)
                {
                    PSEC_WINNT_AUTH_IDENTITY_W auth_data = pAuthData;

                    TRACE("Username is %s\n", debugstr_wn(auth_data->User, auth_data->UserLength));
                    TRACE("Domain name is %s\n", debugstr_wn(auth_data->Domain, auth_data->DomainLength));

                    //cred->username_arg = GetUsernameArg(auth_data->User, auth_data->UserLength);
                    //cred->domain_arg = GetDomainArg(auth_data->Domain, auth_data->DomainLength);
                }

                phCredential->dwUpper = fCredentialUse;
                phCredential->dwLower = (ULONG_PTR)cred;
                TRACE("ACH phCredential->dwUpper: 0x%08lx, dwLower: 0x%08lx\n",
                      phCredential->dwUpper, phCredential->dwLower);
                ret = SEC_E_OK;
                break;
            }
        case SECPKG_CRED_BOTH:
            FIXME("AcquireCredentialsHandle: SECPKG_CRED_BOTH stub\n");
            ret = SEC_E_UNSUPPORTED_FUNCTION;
            phCredential = NULL;
            break;
        default:
            phCredential = NULL;
            ret = SEC_E_UNKNOWN_CREDENTIALS;
    }

    HeapFree(GetProcessHeap(), 0, username);
    HeapFree(GetProcessHeap(), 0, domain);

    return ret;
}


/***********************************************************************
 *              AcquireCredentialsHandleA
 */
SECURITY_STATUS SEC_ENTRY AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    int user_sizeW, domain_sizeW, passwd_sizeW;
    
    SEC_WCHAR *user = NULL, *domain = NULL, *passwd = NULL, *package = NULL;
    
    PSEC_WINNT_AUTH_IDENTITY_W pAuthDataW = NULL;
    PSEC_WINNT_AUTH_IDENTITY_A identity  = NULL;

    ERR("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
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

/***********************************************************************
 *             FreeCredentialsHandle
 */
SECURITY_STATUS SEC_ENTRY FreeCredentialsHandle(
        PCredHandle phCredential)
{
    SECURITY_STATUS ret;

    if(phCredential){
        PNtlmCredentials cred = (PNtlmCredentials) phCredential->dwLower;
        phCredential->dwUpper = 0;
        phCredential->dwLower = 0;
        if (cred->password)
            memset(cred->password, 0, cred->pwlen);
        HeapFree(GetProcessHeap(), 0, cred->password);
        HeapFree(GetProcessHeap(), 0, cred->username_arg);
        HeapFree(GetProcessHeap(), 0, cred->domain_arg);
        HeapFree(GetProcessHeap(), 0, cred);
        ret = SEC_E_OK;
    }
    else
        ret = SEC_E_OK;
    
    return ret;
}
