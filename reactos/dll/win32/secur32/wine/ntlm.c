/*
 * Copyright 2005, 2006 Kai Blin
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
 * This file implements the NTLM security provider.
 */

#include "precomp.h"

#include <assert.h>
#include <stdio.h>
#include <wincred.h>
#include <lm.h>

#include "hmac_md5.h"

#include <wine/unicode.h>
#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define NTLM_MAX_BUF 1904
#define MIN_NTLM_AUTH_MAJOR_VERSION 3
#define MIN_NTLM_AUTH_MINOR_VERSION 0
#ifndef __REACTOS__
#define MIN_NTLM_AUTH_MICRO_VERSION 25
#else
#define MIN_NTLM_AUTH_MICRO_VERSION 23
#endif

static CHAR ntlm_auth[] = "ntlm_auth";

/***********************************************************************
 *              QueryCredentialsAttributesA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryCredentialsAttributesA(
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
 *              QueryCredentialsAttributesW
 */
static SECURITY_STATUS SEC_ENTRY ntlm_QueryCredentialsAttributesW(
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

static char *ntlm_GetUsernameArg(LPCWSTR userW, INT userW_length)
{
    static const char username_arg[] = "--username=";
    char *user;
    int unixcp_size;

    unixcp_size =  WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS,
        userW, userW_length, NULL, 0, NULL, NULL) + sizeof(username_arg);
    user = HeapAlloc(GetProcessHeap(), 0, unixcp_size);
    if (!user) return NULL;
    memcpy(user, username_arg, sizeof(username_arg) - 1);
    WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS, userW, userW_length,
        user + sizeof(username_arg) - 1,
        unixcp_size - sizeof(username_arg) + 1, NULL, NULL);
    user[unixcp_size - 1] = '\0';
    return user;
}

static char *ntlm_GetDomainArg(LPCWSTR domainW, INT domainW_length)
{
    static const char domain_arg[] = "--domain=";
    char *domain;
    int unixcp_size;

    unixcp_size = WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS,
        domainW, domainW_length, NULL, 0,  NULL, NULL) + sizeof(domain_arg);
    domain = HeapAlloc(GetProcessHeap(), 0, unixcp_size);
    if (!domain) return NULL;
    memcpy(domain, domain_arg, sizeof(domain_arg) - 1);
    WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS, domainW,
        domainW_length, domain + sizeof(domain_arg) - 1,
        unixcp_size - sizeof(domain) + 1, NULL, NULL);
    domain[unixcp_size - 1] = '\0';
    return domain;
}

/***********************************************************************
 *              AcquireCredentialsHandleW
 */
SECURITY_STATUS SEC_ENTRY ntlm_AcquireCredentialsHandleW(
 SEC_WCHAR *pszPrincipal, SEC_WCHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    PNtlmCredentials ntlm_cred;
    LPWSTR domain = NULL, user = NULL, password = NULL;
    PSEC_WINNT_AUTH_IDENTITY_W auth_data = NULL;

    TRACE("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
     debugstr_w(pszPrincipal), debugstr_w(pszPackage), fCredentialUse,
     pLogonID, pAuthData, pGetKeyFn, pGetKeyArgument, phCredential, ptsExpiry);

    switch(fCredentialUse)
    {
        case SECPKG_CRED_INBOUND:
            ntlm_cred = HeapAlloc(GetProcessHeap(), 0, sizeof(*ntlm_cred));
            if (!ntlm_cred)
                ret = SEC_E_INSUFFICIENT_MEMORY;
            else
            {
                ntlm_cred->mode = NTLM_SERVER;
                ntlm_cred->username_arg = NULL;
                ntlm_cred->domain_arg = NULL;
                ntlm_cred->password = NULL;
                ntlm_cred->pwlen = 0;
                ntlm_cred->no_cached_credentials = 0;

                phCredential->dwUpper = fCredentialUse;
                phCredential->dwLower = (ULONG_PTR)ntlm_cred;
                ret = SEC_E_OK;
            }
            break;
        case SECPKG_CRED_OUTBOUND:
            {
                auth_data = pAuthData;
                ntlm_cred = HeapAlloc(GetProcessHeap(), 0, sizeof(*ntlm_cred));
                if (!ntlm_cred)
                {
                    ret = SEC_E_INSUFFICIENT_MEMORY;
                    break;
                }
                ntlm_cred->mode = NTLM_CLIENT;
                ntlm_cred->username_arg = NULL;
                ntlm_cred->domain_arg = NULL;
                ntlm_cred->password = NULL;
                ntlm_cred->pwlen = 0;
                ntlm_cred->no_cached_credentials = 0;

                if(pAuthData != NULL)
                {
                    int domain_len = 0, user_len = 0, password_len = 0;

                    if (auth_data->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
                    {
                        if (auth_data->DomainLength)
                        {
                            domain_len = MultiByteToWideChar(CP_ACP, 0, (char *)auth_data->Domain,
                                                             auth_data->DomainLength, NULL, 0);
                            domain = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * domain_len);
                            if(!domain)
                            {
                                ret = SEC_E_INSUFFICIENT_MEMORY;
                                break;
                            }
                            MultiByteToWideChar(CP_ACP, 0, (char *)auth_data->Domain, auth_data->DomainLength,
                                                domain, domain_len);
                        }

                        if (auth_data->UserLength)
                        {
                            user_len = MultiByteToWideChar(CP_ACP, 0, (char *)auth_data->User,
                                                           auth_data->UserLength, NULL, 0);
                            user = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * user_len);
                            if(!user)
                            {
                                ret = SEC_E_INSUFFICIENT_MEMORY;
                                break;
                            }
                            MultiByteToWideChar(CP_ACP, 0, (char *)auth_data->User, auth_data->UserLength,
                                                user, user_len);
                        }

                        if (auth_data->PasswordLength)
                        {
                            password_len = MultiByteToWideChar(CP_ACP, 0,(char *)auth_data->Password,
                                                               auth_data->PasswordLength, NULL, 0);
                            password = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * password_len);
                            if(!password)
                            {
                                ret = SEC_E_INSUFFICIENT_MEMORY;
                                break;
                            }
                            MultiByteToWideChar(CP_ACP, 0, (char *)auth_data->Password, auth_data->PasswordLength,
                                                password, password_len);
                        }
                    }
                    else
                    {
                        domain = auth_data->Domain;
                        domain_len = auth_data->DomainLength;

                        user = auth_data->User;
                        user_len = auth_data->UserLength;

                        password = auth_data->Password;
                        password_len = auth_data->PasswordLength;
                    }

                    TRACE("Username is %s\n", debugstr_wn(user, user_len));
                    TRACE("Domain name is %s\n", debugstr_wn(domain, domain_len));

                    ntlm_cred->username_arg = ntlm_GetUsernameArg(user, user_len);
                    ntlm_cred->domain_arg = ntlm_GetDomainArg(domain, domain_len);

                    if(password_len != 0)
                    {
                        ntlm_cred->pwlen = WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS, password,
                                                               password_len, NULL, 0, NULL, NULL);

                        ntlm_cred->password = HeapAlloc(GetProcessHeap(), 0,
                                                        ntlm_cred->pwlen);
                        if(!ntlm_cred->password)
                        {
                            ret = SEC_E_INSUFFICIENT_MEMORY;
                            break;
                        }

                        WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS, password, password_len,
                                            ntlm_cred->password, ntlm_cred->pwlen, NULL, NULL);
                    }
                }

                phCredential->dwUpper = fCredentialUse;
                phCredential->dwLower = (ULONG_PTR)ntlm_cred;
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

    if (auth_data && auth_data->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI)
    {
        HeapFree(GetProcessHeap(), 0, domain);
        HeapFree(GetProcessHeap(), 0, user);
        HeapFree(GetProcessHeap(), 0, password);
    }
    return ret;
}

/***********************************************************************
 *              AcquireCredentialsHandleA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_AcquireCredentialsHandleA(
 SEC_CHAR *pszPrincipal, SEC_CHAR *pszPackage, ULONG fCredentialUse,
 PLUID pLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn,
 PVOID pGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    int user_sizeW, domain_sizeW, passwd_sizeW;
    
    SEC_WCHAR *user = NULL, *domain = NULL, *passwd = NULL, *package = NULL;
    
    PSEC_WINNT_AUTH_IDENTITY_W pAuthDataW = NULL;
    PSEC_WINNT_AUTH_IDENTITY_A identity  = NULL;

    TRACE("(%s, %s, 0x%08x, %p, %p, %p, %p, %p, %p)\n",
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
    
    ret = ntlm_AcquireCredentialsHandleW(NULL, package, fCredentialUse, 
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

/*************************************************************************
 *             ntlm_GetTokenBufferIndex
 * Calculates the index of the secbuffer with BufferType == SECBUFFER_TOKEN
 * Returns index if found or -1 if not found.
 */
static int ntlm_GetTokenBufferIndex(PSecBufferDesc pMessage)
{
    UINT i;

    TRACE("%p\n", pMessage);

    for( i = 0; i < pMessage->cBuffers; ++i )
    {
        if(pMessage->pBuffers[i].BufferType == SECBUFFER_TOKEN)
            return i;
    }

    return -1;
}

/*************************************************************************
 *             ntlm_GetDataBufferIndex
 * Calculates the index of the first secbuffer with BufferType == SECBUFFER_DATA
 * Returns index if found or -1 if not found.
 */
static int ntlm_GetDataBufferIndex(PSecBufferDesc pMessage)
{
    UINT i;

    TRACE("%p\n", pMessage);

    for( i = 0; i < pMessage->cBuffers; ++i )
    {
        if(pMessage->pBuffers[i].BufferType == SECBUFFER_DATA)
            return i;
    }

    return -1;
}

static BOOL ntlm_GetCachedCredential(const SEC_WCHAR *pszTargetName, PCREDENTIALW *cred)
{
    LPCWSTR p;
    LPCWSTR pszHost;
    LPWSTR pszHostOnly;
    BOOL ret;

    if (!pszTargetName)
        return FALSE;

    /* try to get the start of the hostname from service principal name (SPN) */
    pszHost = strchrW(pszTargetName, '/');
    if (pszHost)
    {
        /* skip slash character */
        pszHost++;

        /* find end of host by detecting start of instance port or start of referrer */
        p = strchrW(pszHost, ':');
        if (!p)
            p = strchrW(pszHost, '/');
        if (!p)
            p = pszHost + strlenW(pszHost);
    }
    else /* otherwise not an SPN, just a host */
    {
        pszHost = pszTargetName;
        p = pszHost + strlenW(pszHost);
    }

    pszHostOnly = HeapAlloc(GetProcessHeap(), 0, (p - pszHost + 1) * sizeof(WCHAR));
    if (!pszHostOnly)
        return FALSE;

    memcpy(pszHostOnly, pszHost, (p - pszHost) * sizeof(WCHAR));
    pszHostOnly[p - pszHost] = '\0';

    ret = CredReadW(pszHostOnly, CRED_TYPE_DOMAIN_PASSWORD, 0, cred);

    HeapFree(GetProcessHeap(), 0, pszHostOnly);
    return ret;
}

/***********************************************************************
 *              InitializeSecurityContextW
 */
SECURITY_STATUS SEC_ENTRY ntlm_InitializeSecurityContextW(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR *pszTargetName, 
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
 PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    PNtlmCredentials ntlm_cred;
    PNegoHelper helper = NULL;
    ULONG ctxt_attr = 0;
    char* buffer, *want_flags = NULL;
    PBYTE bin;
    int buffer_len, bin_len, max_len = NTLM_MAX_BUF;
    int token_idx;
    SEC_CHAR *username = NULL;
    SEC_CHAR *domain = NULL;
    SEC_CHAR *password = NULL;

    TRACE("%p %p %s 0x%08x %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    /****************************************
     * When communicating with the client, there can be the
     * following reply packets:
     * YR <base64 blob>         should be sent to the server
     * PW                       should be sent back to helper with
     *                          base64 encoded password
     * AF <base64 blob>         client is done, blob should be
     *                          sent to server with KK prefixed
     * GF <string list>         A string list of negotiated flags
     * GK <base64 blob>         base64 encoded session key
     * BH <char reason>         something broke
     */
    /* The squid cache size is 2010 chars, and that's what ntlm_auth uses */

    if(TargetDataRep == SECURITY_NETWORK_DREP){
        TRACE("Setting SECURITY_NETWORK_DREP\n");
    }

    buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(char) * NTLM_MAX_BUF);
    bin = HeapAlloc(GetProcessHeap(), 0, sizeof(BYTE) * NTLM_MAX_BUF);

    if((phContext == NULL) && (pInput == NULL))
    {
        static char helper_protocol[] = "--helper-protocol=ntlmssp-client-1";
        static CHAR credentials_argv[] = "--use-cached-creds";
        SEC_CHAR *client_argv[5];
        int pwlen = 0;

        TRACE("First time in ISC()\n");

        if(!phCredential)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto isc_end;
        }

        /* As the server side of sspi never calls this, make sure that
         * the handler is a client handler.
         */
        ntlm_cred = (PNtlmCredentials)phCredential->dwLower;
        if(ntlm_cred->mode != NTLM_CLIENT)
        {
            TRACE("Cred mode = %d\n", ntlm_cred->mode);
            ret = SEC_E_INVALID_HANDLE;
            goto isc_end;
        }

        client_argv[0] = ntlm_auth;
        client_argv[1] = helper_protocol;
        if (!ntlm_cred->username_arg && !ntlm_cred->domain_arg)
        {
            LPWKSTA_USER_INFO_1 ui = NULL;
            NET_API_STATUS status;
            PCREDENTIALW cred;

            if (ntlm_GetCachedCredential(pszTargetName, &cred))
            {
                LPWSTR p;
                p = strchrW(cred->UserName, '\\');
                if (p)
                {
                    domain = ntlm_GetDomainArg(cred->UserName, p - cred->UserName);
                    p++;
                }
                else
                {
                    domain = ntlm_GetDomainArg(NULL, 0);
                    p = cred->UserName;
                }

                username = ntlm_GetUsernameArg(p, -1);

                if(cred->CredentialBlobSize != 0)
                {
                    pwlen = WideCharToMultiByte(CP_UNIXCP,
                        WC_NO_BEST_FIT_CHARS, (LPWSTR)cred->CredentialBlob,
                        cred->CredentialBlobSize / sizeof(WCHAR), NULL, 0,
                        NULL, NULL);

                    password = HeapAlloc(GetProcessHeap(), 0, pwlen);

                    WideCharToMultiByte(CP_UNIXCP, WC_NO_BEST_FIT_CHARS,
                                        (LPWSTR)cred->CredentialBlob,
                                        cred->CredentialBlobSize / sizeof(WCHAR),
                                        password, pwlen, NULL, NULL);
                }

                CredFree(cred);

                client_argv[2] = username;
                client_argv[3] = domain;
                client_argv[4] = NULL;
            }
            else
            {
                status = NetWkstaUserGetInfo(NULL, 1, (LPBYTE *)&ui);
                if (status != NERR_Success || ui == NULL || ntlm_cred->no_cached_credentials)
                {
                    ret = SEC_E_NO_CREDENTIALS;
                    goto isc_end;
                }
                username = ntlm_GetUsernameArg(ui->wkui1_username, -1);
                NetApiBufferFree(ui);

                TRACE("using cached credentials\n");

                client_argv[2] = username;
                client_argv[3] = credentials_argv;
                client_argv[4] = NULL;
            }
        }
        else
        {
            client_argv[2] = ntlm_cred->username_arg;
            client_argv[3] = ntlm_cred->domain_arg;
            client_argv[4] = NULL;
        }

        if((ret = fork_helper(&helper, ntlm_auth, client_argv)) != SEC_E_OK)
            goto isc_end;

        helper->mode = NTLM_CLIENT;
        helper->session_key = HeapAlloc(GetProcessHeap(), 0, 16);
        if (!helper->session_key)
        {
            cleanup_helper(helper);
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto isc_end;
        }

        /* Generate the dummy session key = MD4(MD4(password))*/
        if(password || ntlm_cred->password)
        {
            SEC_WCHAR *unicode_password;
            int passwd_lenW;

            TRACE("Converting password to unicode.\n");
            passwd_lenW = MultiByteToWideChar(CP_ACP, 0,
                                              password ? password : ntlm_cred->password,
                                              password ? pwlen : ntlm_cred->pwlen,
                                              NULL, 0);
            unicode_password = HeapAlloc(GetProcessHeap(), 0,
                                         passwd_lenW * sizeof(SEC_WCHAR));
            MultiByteToWideChar(CP_ACP, 0, password ? password : ntlm_cred->password,
                                password ? pwlen : ntlm_cred->pwlen, unicode_password, passwd_lenW);

            SECUR32_CreateNTLM1SessionKey((PBYTE)unicode_password,
                                          passwd_lenW * sizeof(SEC_WCHAR), helper->session_key);

            HeapFree(GetProcessHeap(), 0, unicode_password);
        }
        else
            memset(helper->session_key, 0, 16);

        /* Allocate space for a maximal string of
         * "SF NTLMSSP_FEATURE_SIGN NTLMSSP_FEATURE_SEAL
         * NTLMSSP_FEATURE_SESSION_KEY"
         */
        want_flags = HeapAlloc(GetProcessHeap(), 0, 73);
        if(want_flags == NULL)
        {
            cleanup_helper(helper);
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto isc_end;
        }
        lstrcpyA(want_flags, "SF");
        if(fContextReq & ISC_REQ_CONFIDENTIALITY)
        {
            if(strstr(want_flags, "NTLMSSP_FEATURE_SEAL") == NULL)
                lstrcatA(want_flags, " NTLMSSP_FEATURE_SEAL");
        }
        if(fContextReq & ISC_REQ_CONNECTION)
            ctxt_attr |= ISC_RET_CONNECTION;
        if(fContextReq & ISC_REQ_EXTENDED_ERROR)
            ctxt_attr |= ISC_RET_EXTENDED_ERROR;
        if(fContextReq & ISC_REQ_INTEGRITY)
        {
            if(strstr(want_flags, "NTLMSSP_FEATURE_SIGN") == NULL)
                lstrcatA(want_flags, " NTLMSSP_FEATURE_SIGN");
        }
        if(fContextReq & ISC_REQ_MUTUAL_AUTH)
            ctxt_attr |= ISC_RET_MUTUAL_AUTH;
        if(fContextReq & ISC_REQ_REPLAY_DETECT)
        {
            if(strstr(want_flags, "NTLMSSP_FEATURE_SIGN") == NULL)
                lstrcatA(want_flags, " NTLMSSP_FEATURE_SIGN");
        }
        if(fContextReq & ISC_REQ_SEQUENCE_DETECT)
        {
            if(strstr(want_flags, "NTLMSSP_FEATURE_SIGN") == NULL)
                lstrcatA(want_flags, " NTLMSSP_FEATURE_SIGN");
        }
        if(fContextReq & ISC_REQ_STREAM)
            FIXME("ISC_REQ_STREAM\n");
        if(fContextReq & ISC_REQ_USE_DCE_STYLE)
            ctxt_attr |= ISC_RET_USED_DCE_STYLE;
        if(fContextReq & ISC_REQ_DELEGATE)
            ctxt_attr |= ISC_RET_DELEGATE;

        /* If no password is given, try to use cached credentials. Fall back to an empty
         * password if this failed. */
        if(!password && !ntlm_cred->password)
        {
            lstrcpynA(buffer, "OK", max_len-1);
            if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            {
                cleanup_helper(helper);
                goto isc_end;
            }
            /* If the helper replied with "PW", using cached credentials failed */
            if(!strncmp(buffer, "PW", 2))
            {
                TRACE("Using cached credentials failed.\n");
                lstrcpynA(buffer, "PW AA==", max_len-1);
            }
            else /* Just do a noop on the next run */
                lstrcpynA(buffer, "OK", max_len-1);
        }
        else
        {
            lstrcpynA(buffer, "PW ", max_len-1);
            if((ret = encodeBase64(password ? (unsigned char *)password : (unsigned char *)ntlm_cred->password,
                        password ? pwlen : ntlm_cred->pwlen, buffer+3,
                        max_len-3, &buffer_len)) != SEC_E_OK)
            {
                cleanup_helper(helper);
                goto isc_end;
            }

        }

        TRACE("Sending to helper: %s\n", debugstr_a(buffer));
        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
        {
            cleanup_helper(helper);
            goto isc_end;
        }

        TRACE("Helper returned %s\n", debugstr_a(buffer));

        if(lstrlenA(want_flags) > 2)
        {
            TRACE("Want flags are %s\n", debugstr_a(want_flags));
            lstrcpynA(buffer, want_flags, max_len-1);
            if((ret = run_helper(helper, buffer, max_len, &buffer_len)) 
                    != SEC_E_OK)
            {
                cleanup_helper(helper);
                goto isc_end;
            }
            if(!strncmp(buffer, "BH", 2))
                ERR("Helper doesn't understand new command set. Expect more things to fail.\n");
        }

        lstrcpynA(buffer, "YR", max_len-1);

        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
        {
            cleanup_helper(helper);
            goto isc_end;
        }

        TRACE("%s\n", buffer);

        if(strncmp(buffer, "YR ", 3) != 0)
        {
            /* Something borked */
            TRACE("Helper returned %c%c\n", buffer[0], buffer[1]);
            ret = SEC_E_INTERNAL_ERROR;
            cleanup_helper(helper);
            goto isc_end;
        }
        if((ret = decodeBase64(buffer+3, buffer_len-3, bin,
                        max_len-1, &bin_len)) != SEC_E_OK)
        {
            cleanup_helper(helper);
            goto isc_end;
        }

        /* put the decoded client blob into the out buffer */

        phNewContext->dwUpper = ctxt_attr;
        phNewContext->dwLower = (ULONG_PTR)helper;

        ret = SEC_I_CONTINUE_NEEDED;
    }
    else
    {
        int input_token_idx;

        /* handle second call here */
        /* encode server data to base64 */
        if (!pInput || ((input_token_idx = ntlm_GetTokenBufferIndex(pInput)) == -1))
        {
            ret = SEC_E_INVALID_TOKEN;
            goto isc_end;
        }

        if(!phContext)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto isc_end;
        }

        /* As the server side of sspi never calls this, make sure that
         * the handler is a client handler.
         */
        helper = (PNegoHelper)phContext->dwLower;
        if(helper->mode != NTLM_CLIENT)
        {
            TRACE("Helper mode = %d\n", helper->mode);
            ret = SEC_E_INVALID_HANDLE;
            goto isc_end;
        }

        if (!pInput->pBuffers[input_token_idx].pvBuffer)
        {
            ret = SEC_E_INTERNAL_ERROR;
            goto isc_end;
        }

        if(pInput->pBuffers[input_token_idx].cbBuffer > max_len)
        {
            TRACE("pInput->pBuffers[%d].cbBuffer is: %d\n",
                    input_token_idx,
                    pInput->pBuffers[input_token_idx].cbBuffer);
            ret = SEC_E_INVALID_TOKEN;
            goto isc_end;
        }
        else
            bin_len = pInput->pBuffers[input_token_idx].cbBuffer;

        memcpy(bin, pInput->pBuffers[input_token_idx].pvBuffer, bin_len);

        lstrcpynA(buffer, "TT ", max_len-1);

        if((ret = encodeBase64(bin, bin_len, buffer+3,
                        max_len-3, &buffer_len)) != SEC_E_OK)
            goto isc_end;

        TRACE("Server sent: %s\n", debugstr_a(buffer));

        /* send TT base64 blob to ntlm_auth */
        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            goto isc_end;

        TRACE("Helper replied: %s\n", debugstr_a(buffer));

        if( (strncmp(buffer, "KK ", 3) != 0) &&
                (strncmp(buffer, "AF ", 3) !=0))
        {
            TRACE("Helper returned %c%c\n", buffer[0], buffer[1]);
            ret = SEC_E_INVALID_TOKEN;
            goto isc_end;
        }

        /* decode the blob and send it to server */
        if((ret = decodeBase64(buffer+3, buffer_len-3, bin, max_len,
                        &bin_len)) != SEC_E_OK)
        {
            goto isc_end;
        }

        phNewContext->dwUpper = ctxt_attr;
        phNewContext->dwLower = (ULONG_PTR)helper;

        ret = SEC_E_OK;
    }

    /* put the decoded client blob into the out buffer */

    if (!pOutput || ((token_idx = ntlm_GetTokenBufferIndex(pOutput)) == -1))
    {
        TRACE("no SECBUFFER_TOKEN buffer could be found\n");
        ret = SEC_E_BUFFER_TOO_SMALL;
        if ((phContext == NULL) && (pInput == NULL))
        {
            cleanup_helper(helper);
            phNewContext->dwUpper = 0;
            phNewContext->dwLower = 0;
        }
        goto isc_end;
    }

    if (fContextReq & ISC_REQ_ALLOCATE_MEMORY)
    {
        pOutput->pBuffers[token_idx].pvBuffer = HeapAlloc(GetProcessHeap(), 0, bin_len);
        pOutput->pBuffers[token_idx].cbBuffer = bin_len;
    }
    else if (pOutput->pBuffers[token_idx].cbBuffer < bin_len)
    {
        TRACE("out buffer is NULL or has not enough space\n");
        ret = SEC_E_BUFFER_TOO_SMALL;
        if ((phContext == NULL) && (pInput == NULL))
        {
            cleanup_helper(helper);
            phNewContext->dwUpper = 0;
            phNewContext->dwLower = 0;
        }
        goto isc_end;
    }

    if (!pOutput->pBuffers[token_idx].pvBuffer)
    {
        TRACE("out buffer is NULL\n");
        ret = SEC_E_INTERNAL_ERROR;
        if ((phContext == NULL) && (pInput == NULL))
        {
            cleanup_helper(helper);
            phNewContext->dwUpper = 0;
            phNewContext->dwLower = 0;
        }
        goto isc_end;
    }

    pOutput->pBuffers[token_idx].cbBuffer = bin_len;
    memcpy(pOutput->pBuffers[token_idx].pvBuffer, bin, bin_len);

    if(ret == SEC_E_OK)
    {
        TRACE("Getting negotiated flags\n");
        lstrcpynA(buffer, "GF", max_len - 1);
        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            goto isc_end;

        if(buffer_len < 3)
        {
            TRACE("No flags negotiated.\n");
            helper->neg_flags = 0l;
        }
        else
        {
            TRACE("Negotiated %s\n", debugstr_a(buffer));
            sscanf(buffer + 3, "%lx", &(helper->neg_flags));
            TRACE("Stored 0x%08x as flags\n", helper->neg_flags);
        }

        TRACE("Getting session key\n");
        lstrcpynA(buffer, "GK", max_len - 1);
        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            goto isc_end;

        if(strncmp(buffer, "BH", 2) == 0)
            TRACE("No key negotiated.\n");
        else if(strncmp(buffer, "GK ", 3) == 0)
        {
            if((ret = decodeBase64(buffer+3, buffer_len-3, bin, max_len, 
                            &bin_len)) != SEC_E_OK)
            {
                TRACE("Failed to decode session key\n");
            }
            TRACE("Session key is %s\n", debugstr_a(buffer+3));
            HeapFree(GetProcessHeap(), 0, helper->session_key);
            helper->session_key = HeapAlloc(GetProcessHeap(), 0, bin_len);
            if(!helper->session_key)
            {
                ret = SEC_E_INSUFFICIENT_MEMORY;
                goto isc_end;
            }
            memcpy(helper->session_key, bin, bin_len);
        }

        helper->crypt.ntlm.a4i = SECUR32_arc4Alloc();
        SECUR32_arc4Init(helper->crypt.ntlm.a4i, helper->session_key, 16);
        helper->crypt.ntlm.seq_num = 0l;
        SECUR32_CreateNTLM2SubKeys(helper);
        helper->crypt.ntlm2.send_a4i = SECUR32_arc4Alloc();
        helper->crypt.ntlm2.recv_a4i = SECUR32_arc4Alloc();
        SECUR32_arc4Init(helper->crypt.ntlm2.send_a4i,
                         helper->crypt.ntlm2.send_seal_key, 16);
        SECUR32_arc4Init(helper->crypt.ntlm2.recv_a4i,
                         helper->crypt.ntlm2.recv_seal_key, 16);
        helper->crypt.ntlm2.send_seq_no = 0l;
        helper->crypt.ntlm2.recv_seq_no = 0l;
    }

isc_end:
    HeapFree(GetProcessHeap(), 0, username);
    HeapFree(GetProcessHeap(), 0, domain);
    HeapFree(GetProcessHeap(), 0, password);
    HeapFree(GetProcessHeap(), 0, want_flags);
    HeapFree(GetProcessHeap(), 0, buffer);
    HeapFree(GetProcessHeap(), 0, bin);
    return ret;
}

/***********************************************************************
 *              InitializeSecurityContextA
 */
static SECURITY_STATUS SEC_ENTRY ntlm_InitializeSecurityContextA(
 PCredHandle phCredential, PCtxtHandle phContext, SEC_CHAR *pszTargetName,
 ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, 
 PSecBufferDesc pInput,ULONG Reserved2, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    SEC_WCHAR *target = NULL;

    TRACE("%p %p %s %d %d %d %p %d %p %p %p %p\n", phCredential, phContext,
     debugstr_a(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    if(pszTargetName != NULL)
    {
        int target_size = MultiByteToWideChar(CP_ACP, 0, pszTargetName,
            strlen(pszTargetName)+1, NULL, 0);
        target = HeapAlloc(GetProcessHeap(), 0, target_size *
                sizeof(SEC_WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pszTargetName, strlen(pszTargetName)+1,
            target, target_size);
    }

    ret = ntlm_InitializeSecurityContextW(phCredential, phContext, target,
            fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
            phNewContext, pOutput, pfContextAttr, ptsExpiry);

    HeapFree(GetProcessHeap(), 0, target);
    return ret;
}

/***********************************************************************
 *              AcceptSecurityContext
 */
SECURITY_STATUS SEC_ENTRY ntlm_AcceptSecurityContext(
 PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput,
 ULONG fContextReq, ULONG TargetDataRep, PCtxtHandle phNewContext, 
 PSecBufferDesc pOutput, ULONG *pfContextAttr, PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    char *buffer, *want_flags = NULL;
    PBYTE bin;
    int buffer_len, bin_len, max_len = NTLM_MAX_BUF;
    ULONG ctxt_attr = 0;
    PNegoHelper helper;
    PNtlmCredentials ntlm_cred;

    TRACE("%p %p %p %d %d %p %p %p %p\n", phCredential, phContext, pInput,
     fContextReq, TargetDataRep, phNewContext, pOutput, pfContextAttr,
     ptsExpiry);

    buffer = HeapAlloc(GetProcessHeap(), 0, sizeof(char) * NTLM_MAX_BUF);
    bin    = HeapAlloc(GetProcessHeap(),0, sizeof(BYTE) * NTLM_MAX_BUF);

    if(TargetDataRep == SECURITY_NETWORK_DREP){
        TRACE("Using SECURITY_NETWORK_DREP\n");
    }

    if(phContext == NULL)
    {
        static CHAR server_helper_protocol[] = "--helper-protocol=squid-2.5-ntlmssp";
        SEC_CHAR *server_argv[] = { ntlm_auth,
            server_helper_protocol,
            NULL };

        if (!phCredential)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto asc_end;
        }

        ntlm_cred = (PNtlmCredentials)phCredential->dwLower;

        if(ntlm_cred->mode != NTLM_SERVER)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto asc_end;
        }

        /* This is the first call to AcceptSecurityHandle */
        if(pInput == NULL)
        {
            ret = SEC_E_INCOMPLETE_MESSAGE;
            goto asc_end;
        }

        if(pInput->cBuffers < 1)
        {
            ret = SEC_E_INCOMPLETE_MESSAGE;
            goto asc_end;
        }

        if(pInput->pBuffers[0].cbBuffer > max_len)
        {
            ret = SEC_E_INVALID_TOKEN;
            goto asc_end;
        }
        else
            bin_len = pInput->pBuffers[0].cbBuffer;

        if( (ret = fork_helper(&helper, ntlm_auth, server_argv)) !=
            SEC_E_OK)
        {
            ret = SEC_E_INTERNAL_ERROR;
            goto asc_end;
        }
        helper->mode = NTLM_SERVER;

        /* Handle all the flags */
        want_flags = HeapAlloc(GetProcessHeap(), 0, 73);
        if(want_flags == NULL)
        {
            TRACE("Failed to allocate memory for the want_flags!\n");
            ret = SEC_E_INSUFFICIENT_MEMORY;
            cleanup_helper(helper);
            goto asc_end;
        }
        lstrcpyA(want_flags, "SF");
        if(fContextReq & ASC_REQ_ALLOCATE_MEMORY)
        {
            FIXME("ASC_REQ_ALLOCATE_MEMORY stub\n");
        }
        if(fContextReq & ASC_REQ_CONFIDENTIALITY)
        {
            lstrcatA(want_flags, " NTLMSSP_FEATURE_SEAL");
        }
        if(fContextReq & ASC_REQ_CONNECTION)
        {
            /* This is default, so we'll enable it */
            lstrcatA(want_flags, " NTLMSSP_FEATURE_SESSION_KEY");
            ctxt_attr |= ASC_RET_CONNECTION;
        }
        if(fContextReq & ASC_REQ_EXTENDED_ERROR)
        {
            FIXME("ASC_REQ_EXTENDED_ERROR stub\n");
        }
        if(fContextReq & ASC_REQ_INTEGRITY)
        {
            lstrcatA(want_flags, " NTLMSSP_FEATURE_SIGN");
        }
        if(fContextReq & ASC_REQ_MUTUAL_AUTH)
        {
            FIXME("ASC_REQ_MUTUAL_AUTH stub\n");
        }
        if(fContextReq & ASC_REQ_REPLAY_DETECT)
        {
            FIXME("ASC_REQ_REPLAY_DETECT stub\n");
        }
        if(fContextReq & ISC_REQ_SEQUENCE_DETECT)
        {
            FIXME("ASC_REQ_SEQUENCE_DETECT stub\n");
        }
        if(fContextReq & ISC_REQ_STREAM)
        {
            FIXME("ASC_REQ_STREAM stub\n");
        }
        /* Done with the flags */

        if(lstrlenA(want_flags) > 3)
        {
            TRACE("Server set want_flags: %s\n", debugstr_a(want_flags));
            lstrcpynA(buffer, want_flags, max_len - 1);
            if((ret = run_helper(helper, buffer, max_len, &buffer_len)) !=
                    SEC_E_OK)
            {
                cleanup_helper(helper);
                goto asc_end;
            }
            if(!strncmp(buffer, "BH", 2))
                TRACE("Helper doesn't understand new command set\n");
        }

        /* This is the YR request from the client, encode to base64 */

        memcpy(bin, pInput->pBuffers[0].pvBuffer, bin_len);

        lstrcpynA(buffer, "YR ", max_len-1);

        if((ret = encodeBase64(bin, bin_len, buffer+3, max_len-3,
                    &buffer_len)) != SEC_E_OK)
        {
            cleanup_helper(helper);
            goto asc_end;
        }

        TRACE("Client sent: %s\n", debugstr_a(buffer));

        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) !=
                    SEC_E_OK)
        {
            cleanup_helper(helper);
            goto asc_end;
        }

        TRACE("Reply from ntlm_auth: %s\n", debugstr_a(buffer));
        /* The expected answer is TT <base64 blob> */

        if(strncmp(buffer, "TT ", 3) != 0)
        {
            ret = SEC_E_INTERNAL_ERROR;
            cleanup_helper(helper);
            goto asc_end;
        }

        if((ret = decodeBase64(buffer+3, buffer_len-3, bin, max_len,
                        &bin_len)) != SEC_E_OK)
        {
            cleanup_helper(helper);
            goto asc_end;
        }

        /* send this to the client */
        if(pOutput == NULL)
        {
            ret = SEC_E_INSUFFICIENT_MEMORY;
            cleanup_helper(helper);
            goto asc_end;
        }

        if(pOutput->cBuffers < 1)
        {
            ret = SEC_E_INSUFFICIENT_MEMORY;
            cleanup_helper(helper);
            goto asc_end;
        }

        pOutput->pBuffers[0].cbBuffer = bin_len;
        pOutput->pBuffers[0].BufferType = SECBUFFER_DATA;
        memcpy(pOutput->pBuffers[0].pvBuffer, bin, bin_len);
        ret = SEC_I_CONTINUE_NEEDED;

    }
    else
    {
        /* we expect a KK request from client */
        if(pInput == NULL)
        {
            ret = SEC_E_INCOMPLETE_MESSAGE;
            goto asc_end;
        }

        if(pInput->cBuffers < 1)
        {
            ret = SEC_E_INCOMPLETE_MESSAGE;
            goto asc_end;
        }

        helper = (PNegoHelper)phContext->dwLower;

        if(helper->mode != NTLM_SERVER)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto asc_end;
        }

        if(pInput->pBuffers[0].cbBuffer > max_len)
        {
            ret = SEC_E_INVALID_TOKEN;
            goto asc_end;
        }
        else
            bin_len = pInput->pBuffers[0].cbBuffer;

        memcpy(bin, pInput->pBuffers[0].pvBuffer, bin_len);

        lstrcpynA(buffer, "KK ", max_len-1);

        if((ret = encodeBase64(bin, bin_len, buffer+3, max_len-3,
                    &buffer_len)) != SEC_E_OK)
        {
            goto asc_end;
        }

        TRACE("Client sent: %s\n", debugstr_a(buffer));

        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) !=
                    SEC_E_OK)
        {
            goto asc_end;
        }

        TRACE("Reply from ntlm_auth: %s\n", debugstr_a(buffer));

        /* At this point, we get a NA if the user didn't authenticate, but a BH
         * if ntlm_auth could not connect to winbindd. Apart from running Wine
         * as root, there is no way to fix this for now, so just handle this as
         * a failed login. */
        if(strncmp(buffer, "AF ", 3) != 0)
        {
            if(strncmp(buffer, "NA ", 3) == 0)
            {
                ret = SEC_E_LOGON_DENIED;
                goto asc_end;
            }
            else
            {
                size_t ntlm_pipe_err_v3_len = strlen("BH NT_STATUS_ACCESS_DENIED");
                size_t ntlm_pipe_err_v4_len = strlen("BH NT_STATUS_UNSUCCESSFUL");

                if( (buffer_len >= ntlm_pipe_err_v3_len &&
                     strncmp(buffer, "BH NT_STATUS_ACCESS_DENIED", ntlm_pipe_err_v3_len) == 0) ||
                    (buffer_len >= ntlm_pipe_err_v4_len &&
                     strncmp(buffer, "BH NT_STATUS_UNSUCCESSFUL", ntlm_pipe_err_v4_len) == 0) )
                {
                    TRACE("Connection to winbindd failed\n");
                    ret = SEC_E_LOGON_DENIED;
                }
                else
                    ret = SEC_E_INTERNAL_ERROR;

                goto asc_end;
            }
        }
        pOutput->pBuffers[0].cbBuffer = 0;

        TRACE("Getting negotiated flags\n");
        lstrcpynA(buffer, "GF", max_len - 1);
        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            goto asc_end;

        if(buffer_len < 3)
        {
            TRACE("No flags negotiated, or helper does not support GF command\n");
        }
        else
        {
            TRACE("Negotiated %s\n", debugstr_a(buffer));
            sscanf(buffer + 3, "%lx", &(helper->neg_flags));
            TRACE("Stored 0x%08x as flags\n", helper->neg_flags);
        }

        TRACE("Getting session key\n");
        lstrcpynA(buffer, "GK", max_len - 1);
        if((ret = run_helper(helper, buffer, max_len, &buffer_len)) != SEC_E_OK)
            goto asc_end;

        if(buffer_len < 3)
            TRACE("Helper does not support GK command\n");
        else
        {
            if(strncmp(buffer, "BH ", 3) == 0)
            {
                TRACE("Helper sent %s\n", debugstr_a(buffer+3));
                HeapFree(GetProcessHeap(), 0, helper->session_key);
                helper->session_key = HeapAlloc(GetProcessHeap(), 0, 16);
                if (!helper->session_key)
                {
                    ret = SEC_E_INSUFFICIENT_MEMORY;
                    goto asc_end;
                }
                /*FIXME: Generate the dummy session key = MD4(MD4(password))*/
                memset(helper->session_key, 0 , 16);
            }
            else if(strncmp(buffer, "GK ", 3) == 0)
            {
                if((ret = decodeBase64(buffer+3, buffer_len-3, bin, max_len, 
                                &bin_len)) != SEC_E_OK)
                {
                    TRACE("Failed to decode session key\n");
                }
                TRACE("Session key is %s\n", debugstr_a(buffer+3));
                HeapFree(GetProcessHeap(), 0, helper->session_key);
                helper->session_key = HeapAlloc(GetProcessHeap(), 0, 16);
                if(!helper->session_key)
                {
                    ret = SEC_E_INSUFFICIENT_MEMORY;
                    goto asc_end;
                }
                memcpy(helper->session_key, bin, 16);
            }
        }
        helper->crypt.ntlm.a4i = SECUR32_arc4Alloc();
        SECUR32_arc4Init(helper->crypt.ntlm.a4i, helper->session_key, 16);
        helper->crypt.ntlm.seq_num = 0l;
    }

    phNewContext->dwUpper = ctxt_attr;
    phNewContext->dwLower = (ULONG_PTR)helper;

asc_end:
    HeapFree(GetProcessHeap(), 0, want_flags);
    HeapFree(GetProcessHeap(), 0, buffer);
    HeapFree(GetProcessHeap(), 0, bin);
    return ret;
}

/***********************************************************************
 *              CompleteAuthToken
 */
static SECURITY_STATUS SEC_ENTRY ntlm_CompleteAuthToken(PCtxtHandle phContext,
 PSecBufferDesc pToken)
{
    /* We never need to call CompleteAuthToken anyway */
    TRACE("%p %p\n", phContext, pToken);
    if (!phContext)
        return SEC_E_INVALID_HANDLE;
    
    return SEC_E_OK;
}

/***********************************************************************
 *              DeleteSecurityContext
 */
SECURITY_STATUS SEC_ENTRY ntlm_DeleteSecurityContext(PCtxtHandle phContext)
{
    PNegoHelper helper;

    TRACE("%p\n", phContext);
    if (!phContext)
        return SEC_E_INVALID_HANDLE;

    helper = (PNegoHelper)phContext->dwLower;

    phContext->dwUpper = 0;
    phContext->dwLower = 0;

    SECUR32_arc4Cleanup(helper->crypt.ntlm.a4i);
    SECUR32_arc4Cleanup(helper->crypt.ntlm2.send_a4i);
    SECUR32_arc4Cleanup(helper->crypt.ntlm2.recv_a4i);
    HeapFree(GetProcessHeap(), 0, helper->crypt.ntlm2.send_sign_key);
    HeapFree(GetProcessHeap(), 0, helper->crypt.ntlm2.send_seal_key);
    HeapFree(GetProcessHeap(), 0, helper->crypt.ntlm2.recv_sign_key);
    HeapFree(GetProcessHeap(), 0, helper->crypt.ntlm2.recv_seal_key);

    cleanup_helper(helper);

    return SEC_E_OK;
}

/***********************************************************************
 *              QueryContextAttributesW
 */
SECURITY_STATUS SEC_ENTRY ntlm_QueryContextAttributesW(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer)
{
    TRACE("%p %d %p\n", phContext, ulAttribute, pBuffer);
    if (!phContext)
        return SEC_E_INVALID_HANDLE;

    switch(ulAttribute)
    {
#define _x(x) case (x) : FIXME(#x" stub\n"); break
        _x(SECPKG_ATTR_ACCESS_TOKEN);
        _x(SECPKG_ATTR_AUTHORITY);
        _x(SECPKG_ATTR_DCE_INFO);
        case SECPKG_ATTR_FLAGS:
        {
            PSecPkgContext_Flags spcf = (PSecPkgContext_Flags)pBuffer;
            PNegoHelper helper = (PNegoHelper)phContext->dwLower;

            spcf->Flags = 0;
            if(helper->neg_flags & NTLMSSP_NEGOTIATE_SIGN)
                spcf->Flags |= ISC_RET_INTEGRITY;
            if(helper->neg_flags & NTLMSSP_NEGOTIATE_SEAL)
                spcf->Flags |= ISC_RET_CONFIDENTIALITY;
            return SEC_E_OK;
        }
        _x(SECPKG_ATTR_KEY_INFO);
        _x(SECPKG_ATTR_LIFESPAN);
        _x(SECPKG_ATTR_NAMES);
        _x(SECPKG_ATTR_NATIVE_NAMES);
        _x(SECPKG_ATTR_NEGOTIATION_INFO);
        _x(SECPKG_ATTR_PACKAGE_INFO);
        _x(SECPKG_ATTR_PASSWORD_EXPIRY);
        _x(SECPKG_ATTR_SESSION_KEY);
        case SECPKG_ATTR_SIZES:
        {
            PSecPkgContext_Sizes spcs = (PSecPkgContext_Sizes)pBuffer;
            spcs->cbMaxToken = NTLM_MAX_BUF;
            spcs->cbMaxSignature = 16;
            spcs->cbBlockSize = 0;
            spcs->cbSecurityTrailer = 16;
            return SEC_E_OK;
        }
        _x(SECPKG_ATTR_STREAM_SIZES);
        _x(SECPKG_ATTR_TARGET_INFORMATION);
#undef _x
        default:
            TRACE("Unknown value %d passed for ulAttribute\n", ulAttribute);
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

/***********************************************************************
 *              QueryContextAttributesA
 */
SECURITY_STATUS SEC_ENTRY ntlm_QueryContextAttributesA(PCtxtHandle phContext,
 ULONG ulAttribute, void *pBuffer)
{
    return ntlm_QueryContextAttributesW(phContext, ulAttribute, pBuffer);
}

/***********************************************************************
 *              ImpersonateSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_ImpersonateSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *              RevertSecurityContext
 */
static SECURITY_STATUS SEC_ENTRY ntlm_RevertSecurityContext(PCtxtHandle phContext)
{
    SECURITY_STATUS ret;

    TRACE("%p\n", phContext);
    if (phContext)
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }
    else
    {
        ret = SEC_E_INVALID_HANDLE;
    }
    return ret;
}

/***********************************************************************
 *             ntlm_CreateSignature
 * As both MakeSignature and VerifySignature need this, but different keys
 * are needed for NTLM2, the logic goes into a helper function.
 * To ensure maximal reusability, we can specify the direction as NTLM_SEND for
 * signing/encrypting and NTLM_RECV for verifying/decrypting. When encrypting,
 * the signature is encrypted after the message was encrypted, so
 * CreateSignature shouldn't do it. In this case, encrypt_sig can be set to
 * false.
 */
static SECURITY_STATUS ntlm_CreateSignature(PNegoHelper helper, PSecBufferDesc pMessage,
        int token_idx, SignDirection direction, BOOL encrypt_sig)
{
    ULONG sign_version = 1;
    UINT i;
    PBYTE sig;
    TRACE("%p, %p, %d, %d, %d\n", helper, pMessage, token_idx, direction,
            encrypt_sig);

    sig = pMessage->pBuffers[token_idx].pvBuffer;

    if(helper->neg_flags & NTLMSSP_NEGOTIATE_NTLM2 &&
            helper->neg_flags & NTLMSSP_NEGOTIATE_SIGN)
    {
        BYTE digest[16];
        BYTE seq_no[4];
        HMAC_MD5_CTX hmac_md5_ctx;

        TRACE("Signing NTLM2 style\n");

        if(direction == NTLM_SEND)
        {
            seq_no[0] = (helper->crypt.ntlm2.send_seq_no >>  0) & 0xff;
            seq_no[1] = (helper->crypt.ntlm2.send_seq_no >>  8) & 0xff;
            seq_no[2] = (helper->crypt.ntlm2.send_seq_no >> 16) & 0xff;
            seq_no[3] = (helper->crypt.ntlm2.send_seq_no >> 24) & 0xff;

            ++(helper->crypt.ntlm2.send_seq_no);

            HMACMD5Init(&hmac_md5_ctx, helper->crypt.ntlm2.send_sign_key, 16);
        }
        else
        {
            seq_no[0] = (helper->crypt.ntlm2.recv_seq_no >>  0) & 0xff;
            seq_no[1] = (helper->crypt.ntlm2.recv_seq_no >>  8) & 0xff;
            seq_no[2] = (helper->crypt.ntlm2.recv_seq_no >> 16) & 0xff;
            seq_no[3] = (helper->crypt.ntlm2.recv_seq_no >> 24) & 0xff;

            ++(helper->crypt.ntlm2.recv_seq_no);

            HMACMD5Init(&hmac_md5_ctx, helper->crypt.ntlm2.recv_sign_key, 16);
        }

        HMACMD5Update(&hmac_md5_ctx, seq_no, 4);
        for( i = 0; i < pMessage->cBuffers; ++i )
        {
            if(pMessage->pBuffers[i].BufferType & SECBUFFER_DATA)
                HMACMD5Update(&hmac_md5_ctx, pMessage->pBuffers[i].pvBuffer,
                        pMessage->pBuffers[i].cbBuffer);
        }

        HMACMD5Final(&hmac_md5_ctx, digest);

        if(encrypt_sig && helper->neg_flags & NTLMSSP_NEGOTIATE_KEY_EXCHANGE)
        {
            if(direction == NTLM_SEND)
                SECUR32_arc4Process(helper->crypt.ntlm2.send_a4i, digest, 8);
            else
                SECUR32_arc4Process(helper->crypt.ntlm2.recv_a4i, digest, 8);
        }

        /* The NTLM2 signature is the sign version */
        sig[ 0] = (sign_version >>  0) & 0xff;
        sig[ 1] = (sign_version >>  8) & 0xff;
        sig[ 2] = (sign_version >> 16) & 0xff;
        sig[ 3] = (sign_version >> 24) & 0xff;
        /* The first 8 bytes of the digest */
        memcpy(sig+4, digest, 8);
        /* And the sequence number */
        memcpy(sig+12, seq_no, 4);

        pMessage->pBuffers[token_idx].cbBuffer = 16;

        return SEC_E_OK;
    }
    if(helper->neg_flags & NTLMSSP_NEGOTIATE_SIGN)
    {
        ULONG crc = 0U;
        TRACE("Signing NTLM1 style\n");

        for(i=0; i < pMessage->cBuffers; ++i)
        {
            if(pMessage->pBuffers[i].BufferType & SECBUFFER_DATA)
            {
                crc = ComputeCrc32(pMessage->pBuffers[i].pvBuffer,
                    pMessage->pBuffers[i].cbBuffer, crc);
            }
        }

        sig[ 0] = (sign_version >>  0) & 0xff;
        sig[ 1] = (sign_version >>  8) & 0xff;
        sig[ 2] = (sign_version >> 16) & 0xff;
        sig[ 3] = (sign_version >> 24) & 0xff;
        memset(sig+4, 0, 4);
        sig[ 8] = (crc >>  0) & 0xff;
        sig[ 9] = (crc >>  8) & 0xff;
        sig[10] = (crc >> 16) & 0xff;
        sig[11] = (crc >> 24) & 0xff;
        sig[12] = (helper->crypt.ntlm.seq_num >>  0) & 0xff;
        sig[13] = (helper->crypt.ntlm.seq_num >>  8) & 0xff;
        sig[14] = (helper->crypt.ntlm.seq_num >> 16) & 0xff;
        sig[15] = (helper->crypt.ntlm.seq_num >> 24) & 0xff;

        ++(helper->crypt.ntlm.seq_num);

        if(encrypt_sig)
            SECUR32_arc4Process(helper->crypt.ntlm.a4i, sig+4, 12);
        return SEC_E_OK;
    }

    if(helper->neg_flags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN || helper->neg_flags == 0)
    {
        TRACE("Creating a dummy signature.\n");
        /* A dummy signature is 0x01 followed by 15 bytes of 0x00 */
        memset(pMessage->pBuffers[token_idx].pvBuffer, 0, 16);
        memset(pMessage->pBuffers[token_idx].pvBuffer, 0x01, 1);
        pMessage->pBuffers[token_idx].cbBuffer = 16;
        return SEC_E_OK;
    }

    return SEC_E_UNSUPPORTED_FUNCTION;
}

/***********************************************************************
 *              MakeSignature
 */
SECURITY_STATUS SEC_ENTRY ntlm_MakeSignature(PCtxtHandle phContext,
    ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    PNegoHelper helper;
    int token_idx;

    TRACE("%p %d %p %d\n", phContext, fQOP, pMessage, MessageSeqNo);
    if (!phContext)
        return SEC_E_INVALID_HANDLE;

    if(fQOP)
        FIXME("Ignoring fQOP 0x%08x\n", fQOP);

    if(MessageSeqNo)
        FIXME("Ignoring MessageSeqNo\n");

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    /* If we didn't find a SECBUFFER_TOKEN type buffer */
    if((token_idx = ntlm_GetTokenBufferIndex(pMessage)) == -1)
        return SEC_E_INVALID_TOKEN;

    if(pMessage->pBuffers[token_idx].cbBuffer < 16)
        return SEC_E_BUFFER_TOO_SMALL;

    helper = (PNegoHelper)phContext->dwLower;
    TRACE("Negotiated flags are: 0x%08x\n", helper->neg_flags);

    return ntlm_CreateSignature(helper, pMessage, token_idx, NTLM_SEND, TRUE);
}

/***********************************************************************
 *              VerifySignature
 */
SECURITY_STATUS SEC_ENTRY ntlm_VerifySignature(PCtxtHandle phContext,
    PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    PNegoHelper helper;
    UINT i;
    int token_idx;
    SECURITY_STATUS ret;
    SecBufferDesc local_desc;
    PSecBuffer     local_buff;
    BYTE          local_sig[16];

    TRACE("%p %p %d %p\n", phContext, pMessage, MessageSeqNo, pfQOP);
    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    if((token_idx = ntlm_GetTokenBufferIndex(pMessage)) == -1)
        return SEC_E_INVALID_TOKEN;

    if(pMessage->pBuffers[token_idx].cbBuffer < 16)
        return SEC_E_BUFFER_TOO_SMALL;

    if(MessageSeqNo)
        FIXME("Ignoring MessageSeqNo\n");

    helper = (PNegoHelper)phContext->dwLower;
    TRACE("Negotiated flags: 0x%08x\n", helper->neg_flags);

    local_buff = HeapAlloc(GetProcessHeap(), 0, pMessage->cBuffers * sizeof(SecBuffer));

    local_desc.ulVersion = SECBUFFER_VERSION;
    local_desc.cBuffers = pMessage->cBuffers;
    local_desc.pBuffers = local_buff;

    for(i=0; i < pMessage->cBuffers; ++i)
    {
        if(pMessage->pBuffers[i].BufferType == SECBUFFER_TOKEN)
        {
            local_buff[i].BufferType = SECBUFFER_TOKEN;
            local_buff[i].cbBuffer = 16;
            local_buff[i].pvBuffer = local_sig;
        }
        else
        {
            local_buff[i].BufferType = pMessage->pBuffers[i].BufferType;
            local_buff[i].cbBuffer = pMessage->pBuffers[i].cbBuffer;
            local_buff[i].pvBuffer = pMessage->pBuffers[i].pvBuffer;
        }
    }

    if((ret = ntlm_CreateSignature(helper, &local_desc, token_idx, NTLM_RECV, TRUE)) != SEC_E_OK)
        return ret;

    if(memcmp(((PBYTE)local_buff[token_idx].pvBuffer) + 8,
                ((PBYTE)pMessage->pBuffers[token_idx].pvBuffer) + 8, 8))
        ret = SEC_E_MESSAGE_ALTERED;
    else
        ret = SEC_E_OK;

    HeapFree(GetProcessHeap(), 0, local_buff);

    return ret;

}

/***********************************************************************
 *             FreeCredentialsHandle
 */
SECURITY_STATUS SEC_ENTRY ntlm_FreeCredentialsHandle(PCredHandle phCredential)
{
    SECURITY_STATUS ret;

    if(phCredential){
        PNtlmCredentials ntlm_cred = (PNtlmCredentials) phCredential->dwLower;
        phCredential->dwUpper = 0;
        phCredential->dwLower = 0;
        if (ntlm_cred->password)
            memset(ntlm_cred->password, 0, ntlm_cred->pwlen);
        HeapFree(GetProcessHeap(), 0, ntlm_cred->password);
        HeapFree(GetProcessHeap(), 0, ntlm_cred->username_arg);
        HeapFree(GetProcessHeap(), 0, ntlm_cred->domain_arg);
        HeapFree(GetProcessHeap(), 0, ntlm_cred);
        ret = SEC_E_OK;
    }
    else
        ret = SEC_E_OK;
    
    return ret;
}

/***********************************************************************
 *             EncryptMessage
 */
SECURITY_STATUS SEC_ENTRY ntlm_EncryptMessage(PCtxtHandle phContext,
    ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    PNegoHelper helper;
    int token_idx, data_idx;

    TRACE("(%p %d %p %d)\n", phContext, fQOP, pMessage, MessageSeqNo);

    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(fQOP)
        FIXME("Ignoring fQOP\n");

    if(MessageSeqNo)
        FIXME("Ignoring MessageSeqNo\n");

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    if((token_idx = ntlm_GetTokenBufferIndex(pMessage)) == -1)
        return SEC_E_INVALID_TOKEN;

    if((data_idx = ntlm_GetDataBufferIndex(pMessage)) ==-1 )
        return SEC_E_INVALID_TOKEN;

    if(pMessage->pBuffers[token_idx].cbBuffer < 16)
        return SEC_E_BUFFER_TOO_SMALL;

    helper = (PNegoHelper) phContext->dwLower;

    if(helper->neg_flags & NTLMSSP_NEGOTIATE_NTLM2 && 
            helper->neg_flags & NTLMSSP_NEGOTIATE_SEAL)
    { 
        ntlm_CreateSignature(helper, pMessage, token_idx, NTLM_SEND, FALSE);
        SECUR32_arc4Process(helper->crypt.ntlm2.send_a4i,
                pMessage->pBuffers[data_idx].pvBuffer,
                pMessage->pBuffers[data_idx].cbBuffer);

        if(helper->neg_flags & NTLMSSP_NEGOTIATE_KEY_EXCHANGE)
            SECUR32_arc4Process(helper->crypt.ntlm2.send_a4i,
                    ((BYTE *)pMessage->pBuffers[token_idx].pvBuffer)+4, 8);
    }
    else
    {
        PBYTE sig;
        ULONG save_flags;

        /* EncryptMessage always produces real signatures, so make sure
         * NTLMSSP_NEGOTIATE_SIGN is set*/
        save_flags = helper->neg_flags;
        helper->neg_flags |= NTLMSSP_NEGOTIATE_SIGN;
        ntlm_CreateSignature(helper, pMessage, token_idx, NTLM_SEND, FALSE);
        helper->neg_flags = save_flags;

        sig = pMessage->pBuffers[token_idx].pvBuffer;

        SECUR32_arc4Process(helper->crypt.ntlm.a4i,
                pMessage->pBuffers[data_idx].pvBuffer,
                pMessage->pBuffers[data_idx].cbBuffer);
        SECUR32_arc4Process(helper->crypt.ntlm.a4i, sig+4, 12);

        if(helper->neg_flags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN || helper->neg_flags == 0)
            memset(sig+4, 0, 4);
    }
    return SEC_E_OK;
}

/***********************************************************************
 *             DecryptMessage
 */
SECURITY_STATUS SEC_ENTRY ntlm_DecryptMessage(PCtxtHandle phContext,
    PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret;
    ULONG ntlmssp_flags_save;
    PNegoHelper helper;
    int token_idx, data_idx;
    TRACE("(%p %p %d %p)\n", phContext, pMessage, MessageSeqNo, pfQOP);

    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(MessageSeqNo)
        FIXME("Ignoring MessageSeqNo\n");

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    if((token_idx = ntlm_GetTokenBufferIndex(pMessage)) == -1)
        return SEC_E_INVALID_TOKEN;

    if((data_idx = ntlm_GetDataBufferIndex(pMessage)) ==-1)
        return SEC_E_INVALID_TOKEN;

    if(pMessage->pBuffers[token_idx].cbBuffer < 16)
        return SEC_E_BUFFER_TOO_SMALL;

    helper = (PNegoHelper) phContext->dwLower;

    if(helper->neg_flags & NTLMSSP_NEGOTIATE_NTLM2 && helper->neg_flags & NTLMSSP_NEGOTIATE_SEAL)
    {
        SECUR32_arc4Process(helper->crypt.ntlm2.recv_a4i,
                pMessage->pBuffers[data_idx].pvBuffer,
                pMessage->pBuffers[data_idx].cbBuffer);
    }
    else
    {
        SECUR32_arc4Process(helper->crypt.ntlm.a4i,
                pMessage->pBuffers[data_idx].pvBuffer,
                pMessage->pBuffers[data_idx].cbBuffer);
    }

    /* Make sure we use a session key for the signature check, EncryptMessage
     * always does that, even in the dummy case */
    ntlmssp_flags_save = helper->neg_flags;

    helper->neg_flags |= NTLMSSP_NEGOTIATE_SIGN;
    ret = ntlm_VerifySignature(phContext, pMessage, MessageSeqNo, pfQOP);

    helper->neg_flags = ntlmssp_flags_save;

    return ret;
}

static const SecurityFunctionTableA ntlmTableA = {
    1,
    NULL,   /* EnumerateSecurityPackagesA */
    ntlm_QueryCredentialsAttributesA,   /* QueryCredentialsAttributesA */
    ntlm_AcquireCredentialsHandleA,     /* AcquireCredentialsHandleA */
    ntlm_FreeCredentialsHandle,         /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    ntlm_InitializeSecurityContextA,    /* InitializeSecurityContextA */
    ntlm_AcceptSecurityContext,         /* AcceptSecurityContext */
    ntlm_CompleteAuthToken,             /* CompleteAuthToken */
    ntlm_DeleteSecurityContext,         /* DeleteSecurityContext */
    NULL,  /* ApplyControlToken */
    ntlm_QueryContextAttributesA,       /* QueryContextAttributesA */
    ntlm_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    ntlm_RevertSecurityContext,         /* RevertSecurityContext */
    ntlm_MakeSignature,                 /* MakeSignature */
    ntlm_VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoA */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextA */
    NULL,   /* AddCredentialsA */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    ntlm_EncryptMessage,                /* EncryptMessage */
    ntlm_DecryptMessage,                /* DecryptMessage */
    NULL,   /* SetContextAttributesA */
};

static const SecurityFunctionTableW ntlmTableW = {
    1,
    NULL,   /* EnumerateSecurityPackagesW */
    ntlm_QueryCredentialsAttributesW,   /* QueryCredentialsAttributesW */
    ntlm_AcquireCredentialsHandleW,     /* AcquireCredentialsHandleW */
    ntlm_FreeCredentialsHandle,         /* FreeCredentialsHandle */
    NULL,   /* Reserved2 */
    ntlm_InitializeSecurityContextW,    /* InitializeSecurityContextW */
    ntlm_AcceptSecurityContext,         /* AcceptSecurityContext */
    ntlm_CompleteAuthToken,             /* CompleteAuthToken */
    ntlm_DeleteSecurityContext,         /* DeleteSecurityContext */
    NULL,  /* ApplyControlToken */
    ntlm_QueryContextAttributesW,       /* QueryContextAttributesW */
    ntlm_ImpersonateSecurityContext,    /* ImpersonateSecurityContext */
    ntlm_RevertSecurityContext,         /* RevertSecurityContext */
    ntlm_MakeSignature,                 /* MakeSignature */
    ntlm_VerifySignature,               /* VerifySignature */
    FreeContextBuffer,                  /* FreeContextBuffer */
    NULL,   /* QuerySecurityPackageInfoW */
    NULL,   /* Reserved3 */
    NULL,   /* Reserved4 */
    NULL,   /* ExportSecurityContext */
    NULL,   /* ImportSecurityContextW */
    NULL,   /* AddCredentialsW */
    NULL,   /* Reserved8 */
    NULL,   /* QuerySecurityContextToken */
    ntlm_EncryptMessage,                /* EncryptMessage */
    ntlm_DecryptMessage,                /* DecryptMessage */
    NULL,   /* SetContextAttributesW */
};

#define NTLM_COMMENT \
   { 'N', 'T', 'L', 'M', ' ', \
     'S', 'e', 'c', 'u', 'r', 'i', 't', 'y', ' ', \
     'P', 'a', 'c', 'k', 'a', 'g', 'e', 0}

static CHAR ntlm_comment_A[] = NTLM_COMMENT;
static WCHAR ntlm_comment_W[] = NTLM_COMMENT;

#define NTLM_NAME {'N', 'T', 'L', 'M', 0}

static char ntlm_name_A[] = NTLM_NAME;
static WCHAR ntlm_name_W[] = NTLM_NAME;

/* According to Windows, NTLM has the following capabilities.  */
#define CAPS ( \
    SECPKG_FLAG_INTEGRITY  | \
    SECPKG_FLAG_PRIVACY    | \
    SECPKG_FLAG_TOKEN_ONLY | \
    SECPKG_FLAG_CONNECTION | \
    SECPKG_FLAG_MULTI_REQUIRED    | \
    SECPKG_FLAG_IMPERSONATION     | \
    SECPKG_FLAG_ACCEPT_WIN32_NAME | \
    SECPKG_FLAG_NEGOTIABLE        | \
    SECPKG_FLAG_LOGON             | \
    SECPKG_FLAG_RESTRICTED_TOKENS )

static const SecPkgInfoW infoW = {
    CAPS,
    1,
    RPC_C_AUTHN_WINNT,
    NTLM_MAX_BUF,
    ntlm_name_W,
    ntlm_comment_W
};

static const SecPkgInfoA infoA = {
    CAPS,
    1,
    RPC_C_AUTHN_WINNT,
    NTLM_MAX_BUF,
    ntlm_name_A,
    ntlm_comment_A
};

SecPkgInfoA *ntlm_package_infoA = (SecPkgInfoA *)&infoA;
SecPkgInfoW *ntlm_package_infoW = (SecPkgInfoW *)&infoW;

void SECUR32_initNTLMSP(void)
{
    PNegoHelper helper;
    static CHAR version[] = "--version";

    SEC_CHAR *args[] = {
        ntlm_auth,
        version,
        NULL };

    if(fork_helper(&helper, ntlm_auth, args) != SEC_E_OK)
        helper = NULL;
    else
        check_version(helper);

    if( helper &&
        ((helper->major >  MIN_NTLM_AUTH_MAJOR_VERSION) ||
         (helper->major == MIN_NTLM_AUTH_MAJOR_VERSION  &&
          helper->minor >  MIN_NTLM_AUTH_MINOR_VERSION) ||
         (helper->major == MIN_NTLM_AUTH_MAJOR_VERSION  &&
          helper->minor == MIN_NTLM_AUTH_MINOR_VERSION  &&
          helper->micro >= MIN_NTLM_AUTH_MICRO_VERSION)) )
    {
        SecureProvider *provider = SECUR32_addProvider(&ntlmTableA, &ntlmTableW, NULL);
        SECUR32_addPackages(provider, 1L, ntlm_package_infoA, ntlm_package_infoW);
    }
    else
    {
        ERR_(winediag)("%s was not found or is outdated. "
                       "Make sure that ntlm_auth >= %d.%d.%d is in your path. "
                       "Usually, you can find it in the winbind package of your distribution.\n",
                       ntlm_auth,
                       MIN_NTLM_AUTH_MAJOR_VERSION,
                       MIN_NTLM_AUTH_MINOR_VERSION,
                       MIN_NTLM_AUTH_MICRO_VERSION);

    }
    cleanup_helper(helper);
}
