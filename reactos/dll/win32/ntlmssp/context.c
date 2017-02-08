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

PNTLMSSP_CONTEXT
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
    return context;
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

    /* decrement reference */
    context->RefCount--;

    /* check for object rundown */
    if (context->RefCount == 0)
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
                              NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY | //if supported
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
                                   NTLMSSP_NEGOTIATE_KEY_EXCH |
                                   NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;

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
        context->NegotiateFlags |= NTLMSSP_REQUEST_INIT_RESP;
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
    }
    
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

    return ret;

fail:
    /* free resources */
    NtlmDereferenceContext((ULONG_PTR)context);
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
    PSecBuffer InputToken1, InputToken2 = NULL;
    PSecBuffer OutputToken1, OutputToken2 = NULL;
    SecBufferDesc BufferDesc;
    ULONG_PTR newContext = 0;
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

        if(!newContext || !NT_SUCCESS(ret))
        {
            ERR("NtlmCreateNegoContext failed with %lx\n", ret);
            goto fail;
        }

        ret = NtlmGenerateNegotiateMessage(newContext,
                                           fContextReq,
                                           OutputToken1);

        /* set result */
        phNewContext->dwUpper = NegotiateFlags;
        phNewContext->dwLower = newContext;
    }
    else if(phContext)       /* challenge! */
    {
        TRACE("ISC challenged!\n");
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
        *phNewContext = *phContext;
        ret = NtlmHandleChallengeMessage(phNewContext->dwLower,
                                         fContextReq,
                                         InputToken1,
                                         InputToken2,
                                         OutputToken1,
                                         OutputToken2,
                                         pfContextAttr,
                                         ptsExpiry,
                                         &NegotiateFlags);
    }
    else
    {
        ERR("bad call to InitializeSecurityContext\n");
        goto fail;
    }

    if(!NT_SUCCESS(ret))
    {
        ERR("failed with %lx\n", ret);
        goto fail;
    }

    /* build blob with the output message */
    BufferDesc.ulVersion = SECBUFFER_VERSION;
    BufferDesc.cBuffers = 1;
    BufferDesc.pBuffers = OutputToken1;

    if(fContextReq & ISC_REQ_ALLOCATE_MEMORY)
        *pfContextAttr |= ISC_RET_ALLOCATED_MEMORY;

    if(pOutput)
        *pOutput = BufferDesc;

    return ret;

fail:
    /* free resources */
    if(newContext)
        NtlmDereferenceContext(newContext);

    if(fContextReq & ISC_REQ_ALLOCATE_MEMORY)
    {
        if(OutputToken1 && OutputToken1->pvBuffer)
            NtlmFree(OutputToken1->pvBuffer);
        if(OutputToken2 && OutputToken2->pvBuffer)
            NtlmFree(OutputToken2->pvBuffer);
    }

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
    SECURITY_STATUS ret = SEC_E_OK;
    PNTLMSSP_CONTEXT context = NtlmReferenceContext(phContext->dwLower);

    TRACE("%p %lx %p\n", phContext, ulAttribute, pBuffer);

    if (!context)
        return SEC_E_INVALID_HANDLE;

    switch(ulAttribute)
    {
        case SECPKG_ATTR_SIZES:
        {
            PSecPkgContext_Sizes spcs  = (PSecPkgContext_Sizes) pBuffer;
            spcs->cbMaxToken = NTLM_MAX_BUF;
            spcs->cbMaxSignature = sizeof(MESSAGE_SIGNATURE);
            spcs->cbBlockSize = 0;
            spcs->cbSecurityTrailer = sizeof(MESSAGE_SIGNATURE);
            break;
        }
        case SECPKG_ATTR_FLAGS:
        {
            PSecPkgContext_Flags spcf = (PSecPkgContext_Flags)pBuffer;
            spcf->Flags = 0;
            if(context->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN)
                spcf->Flags |= ISC_RET_INTEGRITY;
            if(context->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL)
                spcf->Flags |= ISC_RET_CONFIDENTIALITY;
            break;
        }
    default:
        FIXME("ulAttribute %lx unsupported\n", ulAttribute);
        ret = SEC_E_UNSUPPORTED_FUNCTION;
    }

    NtlmDereferenceContext((ULONG_PTR)context);
    return ret;
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
    PSecBuffer InputToken1, InputToken2 = NULL;
    PSecBuffer OutputToken1, OutputToken2 = NULL;
    SecBufferDesc BufferDesc;
    USER_SESSION_KEY sessionKey;
    ULONG userflags;

    TRACE("AcceptSecurityContext %p %p %p %lx %lx %p %p %p %p\n", phCredential, phContext, pInput,
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
        ERR("Failed to get input token 2!\n");
        //return SEC_E_INVALID_TOKEN;
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

    /* first call */
    if(!phContext)// && !InputToken2)
    {
        if(!phCredential)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto fail;
        }
        TRACE("phNewContext->dwLower %lx\n", phNewContext->dwLower);
        ret = NtlmHandleNegotiateMessage(phCredential->dwLower,
                                         phNewContext->dwLower,
                                         fContextReq,
                                         InputToken1,
                                         InputToken2,
                                         OutputToken1,
                                         OutputToken2,
                                         pfContextAttr,
                                         ptsExpiry);
    }
    else
    {
        *phNewContext = *phContext;
        TRACE("phNewContext->dwLower %lx\n", phNewContext->dwLower);
        ret = NtlmHandleAuthenticateMessage(phNewContext->dwLower,
                                            fContextReq,
                                            InputToken1,
                                            OutputToken1,
                                            pfContextAttr,
                                            ptsExpiry,
                                            (PUCHAR)&sessionKey,
                                            &userflags);
    }

    if(!NT_SUCCESS(ret))
    {
        ERR("failed with %lx\n", ret);
        goto fail;
    }

    /* build blob with the output message */
    BufferDesc.ulVersion = SECBUFFER_VERSION;
    BufferDesc.cBuffers = 1;
    BufferDesc.pBuffers = OutputToken1;

    if(fContextReq & ASC_REQ_ALLOCATE_MEMORY)
        *pfContextAttr |= ASC_RET_ALLOCATED_MEMORY;

    *pOutput = BufferDesc;
    return ret;
fail:
    return ret;
}

SECURITY_STATUS
SEC_ENTRY
DeleteSecurityContext(PCtxtHandle phContext)
{
    if (!phContext)
        return SEC_E_INVALID_HANDLE;

    NtlmDereferenceContext((ULONG_PTR)phContext->dwLower);
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
    NtlmFree(pv);
    return SEC_E_OK;
}

SECURITY_STATUS
SEC_ENTRY
ApplyControlToken(IN PCtxtHandle phContext,
                  IN PSecBufferDesc pInput)
{
    UNIMPLEMENTED;
    return SEC_E_UNSUPPORTED_FUNCTION;
}

