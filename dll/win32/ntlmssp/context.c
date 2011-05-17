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
#include "protocol.h"
#include <lm.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

CRITICAL_SECTION ContextCritSect;
LIST_ENTRY ValidContextList;

NTSTATUS
NtlmContextInitialize(VOID)
{
    InitializeCriticalSection(&ContextCritSect);
    InitializeListHead(&ValidContextList);

    return STATUS_SUCCESS;
}

VOID
NtlmReferenceContext(IN ULONG_PTR Handle)
{
    PNTLMSSP_CONTEXT context;
    EnterCriticalSection(&ContextCritSect);

    context = (PNTLMSSP_CONTEXT)Handle;

    /* sanity */
    ASSERT(context);
    TRACE("%p refcount %lu\n",context, context->RefCount);
    ASSERT(context->RefCount > 0);

    /* A context that is not authenticated is only valid for a 
       pre-determined interval */
#if 0
    if (NtlmHasIntervalElapsed(context->StartTime, context->Timeout))
    {
        if ((context->State != Authenticated) &&
            (context->State != AuthenticateSent) &&
            (context->State != PassedToService))
        {
            WARN("%p has timed out\n", context);
            LeaveCriticalSection(&ContextCritSect);
            return;
        }
    }
#endif
    context->RefCount++;
    LeaveCriticalSection(&ContextCritSect);
}

VOID
NtlmDereferenceContext(IN ULONG_PTR Handle)
{
    PNTLMSSP_CONTEXT context;
    EnterCriticalSection(&ContextCritSect);

    context = (PNTLMSSP_CONTEXT)Handle;

    /* sanity */
    ASSERT(context);
    TRACE("%p refcount %lu\n",context, context->RefCount);
    ASSERT(context->RefCount >= 1);

    /* decrement and check for delete */
    if (context->RefCount-- == 0)
    {
        TRACE("Deleting context %p\n",context);

        /* dereference credential */
        if(context->Credential)
            NtlmDereferenceCredential((ULONG_PTR)context->Credential);

        /* remove from list */
        RemoveEntryList(&context->Entry);

        /* delete object */
        NtlmFree(context);
    }

    LeaveCriticalSection(&ContextCritSect);
}

VOID
NtlmContextTerminate(VOID)
{
    EnterCriticalSection(&ContextCritSect);

    /* dereference all items */
    while (!IsListEmpty(&ValidContextList))
    {
        PNTLMSSP_CONTEXT Context;
        Context = CONTAINING_RECORD(ValidContextList.Flink,
                                    NTLMSSP_CONTEXT,
                                    Entry);

        NtlmDereferenceContext((ULONG_PTR)Context);
    }

    LeaveCriticalSection(&ContextCritSect);

    /* free critical section */
    DeleteCriticalSection(&ContextCritSect);

    return;
}

PNTLMSSP_CONTEXT
NtlmAllocateContext(VOID)
{
    SECPKG_CALL_INFO CallInfo;
    PNTLMSSP_CONTEXT ret;

    ret = (PNTLMSSP_CONTEXT)NtlmAllocate(sizeof(NTLMSSP_CONTEXT));

    if(!ret)
    {
        ERR("allocate context failed!\n");
        return NULL;
    }

    /* set process fields */
    ret->ProcId = GetCurrentProcessId();

    if(inLsaMode)
        if(NtlmLsaFuncTable->GetCallInfo(&CallInfo))
            ret->ProcId = CallInfo.ProcessId;

    ret->RefCount = 1;
    ret->State = Idle;

    (VOID)NtQuerySystemTime(&ret->StartTime);
    ret->Timeout = NTLM_DEFAULT_TIMEOUT;

    /* insert to list */
    EnterCriticalSection(&ContextCritSect);
    InsertHeadList(&ValidContextList, &ret->Entry);
    LeaveCriticalSection(&ContextCritSect);

    TRACE("added context %p\n",ret);
    return ret;
}

BOOL
NtlmGetCachedCredential(const SEC_WCHAR *pszTargetName,
                        PCREDENTIALW *cred)
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

        /* find fail of host by detecting start of instance port or start of referrer */
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

SECURITY_STATUS
NtlmCreateNegoContext(IN ULONG_PTR Credential,
                      IN SEC_WCHAR *pszTargetName,
                      IN ULONG fContextReq,
                      OUT PULONG_PTR phNewContext,
                      OUT PULONG pfContextAttr,
                      OUT PTimeStamp ptsExpiry,
                      OUT PUCHAR pSessionKey,
                      OUT PULONG pfNegotiateFlags)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PNTLMSSP_CONTEXT context = NULL;
    PNTLMSSP_CREDENTIAL cred;

    *pSessionKey = 0;
    *pfNegotiateFlags = 0;

    cred = NtlmReferenceCredential(Credential);
    if ((cred->UseFlags & SECPKG_CRED_OUTBOUND) == 0 )
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        ERR("Invalid credential use!\n");
        goto fail;
    }

    context = NtlmAllocateContext();

    if(!context)
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        ERR("SEC_E_INSUFFICIENT_MEMORY!\n");
        goto fail;
    }

    /* always on features */
    context->NegotiateFlags = NTLMSSP_NEGOTIATE_UNICODE |
                              NTLMSSP_NEGOTIATE_OEM |
                              NTLMSSP_NEGOTIATE_NTLM |
                              NTLMSSP_NEGOTIATE_NTLM2 | //if supported
                              NTLMSSP_REQUEST_TARGET |
                              NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                              NTLMSSP_NEGOTIATE_56 |
                              NTLMSSP_NEGOTIATE_128; // if supported

    /* client requested features */
    if(fContextReq & ISC_REQ_INTEGRITY)
    {
        *pfContextAttr |= ISC_RET_INTEGRITY;
        context->ContextFlags |= ISC_RET_INTEGRITY;
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if(fContextReq & ISC_REQ_SEQUENCE_DETECT)
    {
        *pfContextAttr |= ISC_RET_SEQUENCE_DETECT;
        context->ContextFlags |= ISC_RET_SEQUENCE_DETECT;
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if(fContextReq & ISC_REQ_REPLAY_DETECT)
    {
        *pfContextAttr |= ISC_RET_REPLAY_DETECT;
        context->ContextFlags |= ISC_RET_REPLAY_DETECT;
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if(fContextReq & ISC_REQ_CONFIDENTIALITY)
    {
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_SEAL |
                                   NTLMSSP_NEGOTIATE_LM_KEY |
                                   NTLMSSP_NEGOTIATE_KEY_EXCH;
                                   //NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;

        *pfContextAttr |= ISC_RET_CONFIDENTIALITY;
        context->ContextFlags |= ISC_RET_CONFIDENTIALITY;
    }

    if(fContextReq & ISC_REQ_NULL_SESSION)
    {
        *pfContextAttr |= ISC_RET_NULL_SESSION;
        context->ContextFlags |= ISC_RET_NULL_SESSION;
    }

    if(fContextReq & ISC_REQ_CONNECTION)
    {
        *pfContextAttr |= ISC_RET_CONNECTION;
        context->ContextFlags |= ISC_RET_CONNECTION;
    }

    if(fContextReq & ISC_REQ_IDENTIFY)
    {
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_IDENTIFY;
        *pfContextAttr |= ISC_RET_IDENTIFY;
        context->ContextFlags |= ISC_RET_IDENTIFY;
    }

    if(!(fContextReq & ISC_REQ_DATAGRAM))
    {
        /* datagram flags */
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_DATAGRAM;
        context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_NT_ONLY;
        context->ContextFlags |= ISC_RET_DATAGRAM;
        *pfContextAttr |= ISC_RET_DATAGRAM;
        //*pfNegotiateFlags |= NTLMSSP_APP_SEQ; app provided sequence numbers

        /* generate session key */
        if(context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN |
                                      NTLMSSP_NEGOTIATE_SEAL))
        {
            ret = NtlmGenerateRandomBits(context->SessionKey,
                                         MSV1_0_USER_SESSION_KEY_LENGTH);
            
            if(!NT_SUCCESS(ret))
            {
                ERR("Failed to generate session key!\n");
                goto fail;
            }
        }

        /* local connection */
        if((!cred->DomainName.Buffer &&
            !cred->UserName.Buffer &&
            !cred->Password.Buffer) &&
            cred->SecToken)
        {
            LPWKSTA_USER_INFO_1 ui = NULL;
            NET_API_STATUS status;
            PCREDENTIALW credW;
            context->isLocal = TRUE;

            TRACE("try use local cached credentials\n");

            /* get local credentials */
            if(pszTargetName && NtlmGetCachedCredential(pszTargetName, &credW))
            {
                LPWSTR p;
                p = strchrW(credW->UserName, '\\');
                if(p)
                {
                    TRACE("%s\n",debugstr_w(credW->UserName));
                    TRACE("%s\n", debugstr_w((WCHAR*)(p - credW->UserName)));
                }
                if(credW->CredentialBlobSize != 0)
                {
                    TRACE("%s\n", debugstr_w((WCHAR*)credW->CredentialBlob));
                }
                CredFree(credW);
            }
            else
            {
                status = NetWkstaUserGetInfo(NULL, 1, (LPBYTE *)&ui);
                if (status != NERR_Success || ui == NULL)
                {
                    ret = SEC_E_NO_CREDENTIALS;
                    goto fail;
                }
                TRACE("%s",debugstr_w(ui->wkui1_username));
                NetApiBufferFree(ui);
            }
        }
    }//end is datagram
    
    /* generate session key */
    if (context->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
    {
        ret = NtlmGenerateRandomBits(context->SessionKey,
                                     MSV1_0_USER_SESSION_KEY_LENGTH);

        if(!NT_SUCCESS(ret))
        {
            ERR("Failed to generate session key!\n");
            goto fail;
        }
    }

    /* commit results */
    *pfNegotiateFlags = context->NegotiateFlags;

    context->Credential = cred;
    //*ptsExpiry = 
    *phNewContext = (ULONG_PTR)context;

    TRACE("context %p context->NegotiateFlags:\n",context);
    NtlmPrintNegotiateFlags(*pfNegotiateFlags);

    return ret;

fail:
    return ret;
}

/* public functions */

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextW(IN OPTIONAL PCredHandle phCredential,
                           IN OPTIONAL PCtxtHandle phContext,
                           IN OPTIONAL SEC_WCHAR *pszTargetName,
                           IN ULONG fContextReq,
                           IN ULONG Reserved1,
                           IN ULONG TargetDataRep,
                           IN OPTIONAL PSecBufferDesc pInput,
                           IN ULONG Reserved2,
                           IN OUT OPTIONAL PCtxtHandle phNewContext,
                           IN OUT OPTIONAL PSecBufferDesc pOutput,
                           OUT ULONG *pfContextAttr,
                           OUT OPTIONAL PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PSecBuffer InputToken1, InputToken2;
    PSecBuffer OutputToken1, OutputToken2;
    ULONG_PTR newContext;
    ULONG NegotiateFlags;
    UCHAR sessionKey;

    TRACE("%p %p %s 0x%08lx %lx %lx %p %lx %p %p %p %p\n", phCredential, phContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    if(TargetDataRep == SECURITY_NETWORK_DREP)
        WARN("SECURITY_NETWORK_DREP!!\n");

    /* get first input token */
    ret = NtlmGetSecBuffer(pInput,
                          0,
                          &InputToken1,
                          FALSE);
    if(!ret)
    {
        ERR("Failed to get input token!\n");
        return SEC_E_INVALID_TOKEN;
    }

    /* get first output token */
    ret = NtlmGetSecBuffer(pOutput,
                          0,
                          &OutputToken1,
                          TRUE);
    if(!ret)
    {
        ERR("Failed to get output token!\n");
        return SEC_E_BUFFER_TOO_SMALL;
    }

    /* first call! nego message creation */
    if(!phContext && !pInput)
    {
        if(!phCredential)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto fail;
        }

        ret = NtlmCreateNegoContext(phCredential->dwLower,
                                    pszTargetName,
                                    fContextReq,
                                    &newContext,
                                    pfContextAttr,
                                    ptsExpiry,
                                    &sessionKey,
                                    &NegotiateFlags);

        phNewContext = (PCtxtHandle)newContext;

        if(!newContext || !NT_SUCCESS(ret))
        {
            ERR("NtlmCreateNegoContext failed with %lx\n", ret);
            goto fail;
        }

        ret = NtlmGenerateNegotiateMessage(newContext,
                                           fContextReq,
                                           NegotiateFlags,
                                           InputToken1,
                                           &OutputToken1);

        if(!NT_SUCCESS(ret))
        {
            ERR("NtlmGenerateNegotiateMessage failed with %lx\n", ret);
            goto fail;
        }

        /* build blob with the nego message */
        SecBufferDesc BufferDesc;
        BufferDesc.ulVersion = SECBUFFER_VERSION;
        BufferDesc.cBuffers = 1;
        BufferDesc.pBuffers = OutputToken1;

        if(fContextReq & ISC_REQ_ALLOCATE_MEMORY)
            *pfContextAttr |= ISC_RET_ALLOCATED_MEMORY;

        *pOutput = BufferDesc;

    }
    else        /* challenge! */
    {
        ERR("challenge message unimplemented!!!\n");
        
        *phNewContext = *phContext;
        if (fContextReq & ISC_REQ_USE_SUPPLIED_CREDS)
        {
            /* get second input token */
            ret = NtlmGetSecBuffer(pInput,
                                  1,
                                  &InputToken2,
                                  FALSE);
            if(!ret)
            {
                ERR("Failed to get input token!\n");
                return SEC_E_INVALID_TOKEN;
            }
        }

        /* get second output token */
        ret = NtlmGetSecBuffer(pOutput,
                              1,
                              &OutputToken2,
                              TRUE);
        if(!ret)
        {
            ERR("Failed to get output token!\n");
            return SEC_E_INVALID_TOKEN;
        }

    }
    return ret;

fail:
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextA(IN OPTIONAL PCredHandle phCredential,
                           IN OPTIONAL PCtxtHandle phContext,
                           IN OPTIONAL SEC_CHAR *pszTargetName,
                           IN ULONG fContextReq,
                           IN ULONG Reserved1,
                           IN ULONG TargetDataRep,
                           IN OPTIONAL PSecBufferDesc pInput,
                           IN ULONG Reserved2,
                           IN OUT OPTIONAL PCtxtHandle phNewContext,
                           IN OUT OPTIONAL PSecBufferDesc pOutput,
                           OUT ULONG *pfContextAttr,
                           OUT OPTIONAL PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret;
    SEC_WCHAR *target = NULL;

    TRACE("%p %p %s %lx %lx %lx %p %lx %p %p %p %p\n", phCredential, phContext,
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

    ret = InitializeSecurityContextW(phCredential, phContext, target,
            fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
            phNewContext, pOutput, pfContextAttr, ptsExpiry);

    HeapFree(GetProcessHeap(), 0, target);
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(PCtxtHandle phContext,
                        ULONG ulAttribute,
                        void *pBuffer)
{
    TRACE("%p %lx %p\n", phContext, ulAttribute, pBuffer);
    if (!phContext)
        return SEC_E_INVALID_HANDLE;

    UNIMPLEMENTED;

    return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesA(PCtxtHandle phContext,
                        ULONG ulAttribute,
                        void *pBuffer)
{
    return QueryContextAttributesW(phContext, ulAttribute, pBuffer);
}

SECURITY_STATUS
SEC_ENTRY
AcceptSecurityContext(IN PCredHandle phCredential,
                      IN OUT PCtxtHandle phContext,
                      IN PSecBufferDesc pInput,
                      IN ULONG fContextReq,
                      IN ULONG TargetDataRep,
                      IN OUT PCtxtHandle phNewContext,
                      IN OUT PSecBufferDesc pOutput,
                      OUT ULONG *pfContextAttr,
                      OUT PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PSecBuffer InputToken1, InputToken2;
    PSecBuffer OutputToken1;
    ULONG_PTR newContext;

    TRACE("%p %p %p %lx %lx %p %p %p %p\n", phCredential, phContext, pInput,
        fContextReq, TargetDataRep, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    /* get first input token */
    ret = NtlmGetSecBuffer(pInput,
                          0,
                          &InputToken1,
                          FALSE);
    if(!ret)
    {
        ERR("Failed to get input token!\n");
        return SEC_E_INVALID_TOKEN;
    }

    /* get second input token */
    ret = NtlmGetSecBuffer(pInput,
                          1,
                          &InputToken2,
                          FALSE);
    if(!ret)
    {
        ERR("Failed to get input token!\n");
        return SEC_E_INVALID_TOKEN;
    }

    /* get first output token */
    ret = NtlmGetSecBuffer(pOutput,
                          0,
                          &OutputToken1,
                          TRUE);
    if(!ret)
    {
        ERR("Failed to get output token!\n");
        return SEC_E_BUFFER_TOO_SMALL;
    }

    ERR("here!");
    /* first call */
    if(!phContext && !InputToken2->cbBuffer)
    {
        if(!phCredential)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto fail;
        }

        ret = NtlmHandleNegotiateMessage(phCredential->dwLower,
                                         &newContext,
                                         fContextReq,
                                         InputToken1,
                                         &OutputToken1,
                                         pfContextAttr,
                                         ptsExpiry);
        phNewContext = (PCtxtHandle)newContext;
    }
    else
        WARN("Handle Authenticate UNIMPLEMENTED!\n");

    //if(!NT_SUCCESS(ret))

    UNIMPLEMENTED;
    return ret;
fail:
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
DeleteSecurityContext(PCtxtHandle phContext)
{
    if (!phContext)
    {
        return SEC_E_INVALID_HANDLE;
    }

    NtlmDereferenceContext((ULONG_PTR)phContext);
    phContext = NULL;
    return SEC_E_OK;
}

SECURITY_STATUS
SEC_ENTRY
ImpersonateSecurityContext(PCtxtHandle phContext)
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
SECURITY_STATUS
SEC_ENTRY
RevertSecurityContext(PCtxtHandle phContext)
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

SECURITY_STATUS
SEC_ENTRY
FreeContextBuffer(PVOID pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
    return SEC_E_OK;
}

SECURITY_STATUS
SEC_ENTRY
ApplyControlToken(IN  PCtxtHandle phContext,
                  IN  PSecBufferDesc pInput)
{

    UNIMPLEMENTED;
    return SEC_E_UNSUPPORTED_FUNCTION;
}

