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

PNTLMSSP_CONTEXT_HDR
NtlmReferenceContextHdr(IN ULONG_PTR Handle)
{
    PNTLMSSP_CONTEXT_HDR context;
    EnterCriticalSection(&ContextCritSect);

    context = (PNTLMSSP_CONTEXT_HDR)Handle;

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

PNTLMSSP_CONTEXT_SVR
NtlmReferenceContextSvr(IN ULONG_PTR Handle)
{
    PNTLMSSP_CONTEXT_HDR c = (PNTLMSSP_CONTEXT_HDR)NtlmReferenceContextHdr(Handle);
    ASSERT(c->isServer);
    return (PNTLMSSP_CONTEXT_SVR)c;
}

PNTLMSSP_CONTEXT_CLI
NtlmReferenceContextCli(IN ULONG_PTR Handle)
{
    PNTLMSSP_CONTEXT_HDR c = (PNTLMSSP_CONTEXT_HDR)NtlmReferenceContextHdr(Handle);
    ASSERT(!c->isServer);
    return (PNTLMSSP_CONTEXT_CLI)c;
}
PNTLMSSP_CONTEXT_MSG
NtlmReferenceContextMsg(
    IN ULONG_PTR Handle,
    OUT PULONG pNegFlg,
    OUT prc4_key* pSendHandle,
    OUT prc4_key* pRecvHandle)
{
    PNTLMSSP_CONTEXT_HDR c;
    c = NtlmReferenceContextHdr(Handle);
    if (c->isServer)
    {
        PNTLMSSP_CONTEXT_SVR csvr = (PNTLMSSP_CONTEXT_SVR)c;
        if (pNegFlg)
            *pNegFlg = csvr->cli_NegFlg;
        *pSendHandle = &csvr->cli_msg.ServerHandle;
        *pRecvHandle = &csvr->cli_msg.ClientHandle;
        return &csvr->cli_msg;
    }
    else
    {
        PNTLMSSP_CONTEXT_CLI ccli = (PNTLMSSP_CONTEXT_CLI)c;
        if (pNegFlg)
            *pNegFlg = ccli->NegFlg;
        *pSendHandle = &ccli->msg.ClientHandle;
        *pRecvHandle = &ccli->msg.ServerHandle;
        return &ccli->msg;
    }
}


VOID
NtlmDereferenceContext(IN ULONG_PTR Handle)
{
    PNTLMSSP_CONTEXT_HDR context;
    EnterCriticalSection(&ContextCritSect);

    context = (PNTLMSSP_CONTEXT_HDR)Handle;

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
        if (context->isServer)
        {
            PNTLMSSP_CONTEXT_SVR csvr = (PNTLMSSP_CONTEXT_SVR)context;
            if(csvr->Credential)
                NtlmDereferenceCredential((ULONG_PTR)csvr->Credential);
        }
        else
        {
            PNTLMSSP_CONTEXT_CLI ccli = (PNTLMSSP_CONTEXT_CLI)context;
            if(ccli->Credential)
                NtlmDereferenceCredential((ULONG_PTR)ccli->Credential);
        }

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
        PNTLMSSP_CONTEXT_HDR Context;
        Context = CONTAINING_RECORD(ValidContextList.Flink,
                                    NTLMSSP_CONTEXT_HDR,
                                    Entry);

        NtlmDereferenceContext((ULONG_PTR)Context);
    }

    LeaveCriticalSection(&ContextCritSect);

    /* free critical section */
    DeleteCriticalSection(&ContextCritSect);

    return;
}

PNTLMSSP_CONTEXT_HDR
NtlmAllocateContextHdr(BOOL isServer)
{
    SECPKG_CALL_INFO CallInfo;
    PNTLMSSP_CONTEXT_HDR ret;

    ULONG ctxSize = (isServer) ? sizeof(NTLMSSP_CONTEXT_SVR) :
                                 sizeof(NTLMSSP_CONTEXT_CLI);

    ret = (PNTLMSSP_CONTEXT_HDR)NtlmAllocate(ctxSize);

    if(!ret)
    {
        ERR("allocate context failed!\n");
        return NULL;
    }

    /* set process fields */
    ret->ProcId = GetCurrentProcessId();

    ret->isServer = isServer;

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

PNTLMSSP_CONTEXT_CLI
NtlmAllocateContextCli(VOID)
{
    PNTLMSSP_CONTEXT_CLI ret;

    ret = (PNTLMSSP_CONTEXT_CLI)NtlmAllocateContextHdr(FALSE);
    return ret;
}

PNTLMSSP_CONTEXT_SVR
NtlmAllocateContextSvr(VOID)
{
    PNTLMSSP_CONTEXT_SVR ret;

    ret = (PNTLMSSP_CONTEXT_SVR)NtlmAllocateContextHdr(TRUE);
    /* always on features */
    ret->CfgFlg = NTLMSSP_NEGOTIATE_UNICODE |
                  NTLMSSP_NEGOTIATE_OEM |
                  NTLMSSP_NEGOTIATE_NTLM |
                  NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY | //if supported
                  NTLMSSP_REQUEST_TARGET |
                  NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                  NTLMSSP_NEGOTIATE_56 |
                  NTLMSSP_NEGOTIATE_128; // if supported
    return ret;
}

SECURITY_STATUS
CliCreateContext(
    IN ULONG_PTR Credential,
    IN SEC_WCHAR *pszTargetName,
    IN ULONG ISCContextReq,
    OUT PNTLMSSP_CONTEXT_CLI* pNewContext,
    OUT PULONG pISCContextAttr,
    OUT PTimeStamp ptsExpiry,
    OUT PULONG pfNegotiateFlags)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PNTLMSSP_CONTEXT_CLI context = NULL;
    PNTLMSSP_CREDENTIAL cred;
    PNTLMSSP_GLOBALS_CLI gcli = getGlobalsCli();

    *pfNegotiateFlags = 0;

    /* It seems these flags are always returned */
    *pISCContextAttr = ISC_RET_INTEGRITY;

    cred = NtlmReferenceCredential(Credential);
    if ((cred->UseFlags & SECPKG_CRED_OUTBOUND) == 0 )
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        ERR("Invalid credential use!\n");
        goto fail;
    }

    context = NtlmAllocateContextCli();
    if(!context)
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        ERR("SEC_E_INSUFFICIENT_MEMORY!\n");
        goto fail;
    }
    NtlmReferenceContextCli((ULONG_PTR)context);

    /* configure */
    if (gcli->CfgFlags & NTLMSSP_CLICFGFLAG_NTLMV2_ENABLED)
    {
        context->UseNTLMv2 = TRUE;
    }
    else
    {
        context->UseNTLMv2 = FALSE;
    }

    /* always on features - MS-NLMP 3.1.5.1.1 */
    context->NegFlg = NTLMSSP_REQUEST_TARGET |
                      NTLMSSP_NEGOTIATE_NTLM |
                      NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                      NTLMSSP_NEGOTIATE_UNICODE;

    /* addiditonal flags / features w2k3 returns */
    context->NegFlg = context->NegFlg |
                      NTLMSSP_NEGOTIATE_SEAL |
                      NTLMSSP_NEGOTIATE_56 |
                      NTLMSSP_NEGOTIATE_128 |
                      NTLMSSP_NEGOTIATE_OEM |
                      NTLMSSP_NEGOTIATE_SIGN |
                      NTLMSSP_NEGOTIATE_VERSION |
                      NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY |
                      NTLMSSP_NEGOTIATE_KEY_EXCH;

    /* client requested features */
    if(ISCContextReq & ISC_REQ_INTEGRITY)
    {
        *pISCContextAttr |= ISC_RET_INTEGRITY;
        context->ISCRetContextFlags |= ISC_RET_INTEGRITY;
        context->NegFlg |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if(ISCContextReq & ISC_REQ_SEQUENCE_DETECT)
    {
        *pISCContextAttr |= ISC_RET_SEQUENCE_DETECT;
        context->ISCRetContextFlags |= ISC_RET_SEQUENCE_DETECT;
        context->NegFlg |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if(ISCContextReq & ISC_REQ_REPLAY_DETECT)
    {
        *pISCContextAttr |= ISC_RET_REPLAY_DETECT;
        context->ISCRetContextFlags |= ISC_RET_REPLAY_DETECT;
        context->NegFlg |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if(ISCContextReq & ISC_REQ_CONFIDENTIALITY)
    {
        context->NegFlg |= NTLMSSP_NEGOTIATE_SEAL |
                           NTLMSSP_NEGOTIATE_LM_KEY |
                           NTLMSSP_NEGOTIATE_KEY_EXCH |
                           NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;

        *pISCContextAttr |= ISC_RET_CONFIDENTIALITY;
        context->ISCRetContextFlags |= ISC_RET_CONFIDENTIALITY;
    }

    if(ISCContextReq & ISC_REQ_NULL_SESSION)
    {
        *pISCContextAttr |= ISC_RET_NULL_SESSION;
        context->ISCRetContextFlags |= ISC_RET_NULL_SESSION;
    }

    if(ISCContextReq & ISC_REQ_CONNECTION)
    {
        *pISCContextAttr |= ISC_RET_CONNECTION;
        context->ISCRetContextFlags |= ISC_RET_CONNECTION;
    }

    if(ISCContextReq & ISC_REQ_IDENTIFY)
    {
        context->NegFlg |= NTLMSSP_REQUEST_INIT_RESP;
        *pISCContextAttr |= ISC_RET_IDENTIFY;
        context->ISCRetContextFlags |= ISC_RET_IDENTIFY;
    }

    if (ISCContextReq & ISC_REQ_DATAGRAM)
    {
        /* datagram flags */
        context->NegFlg |= NTLMSSP_NEGOTIATE_DATAGRAM;
        context->NegFlg &= ~NTLMSSP_NEGOTIATE_NT_ONLY;
        context->ISCRetContextFlags |= ISC_RET_DATAGRAM;
        *pISCContextAttr |= ISC_RET_DATAGRAM;

        /* generate session key */
        if(context->NegFlg & (NTLMSSP_NEGOTIATE_SIGN |
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
    if (context->NegFlg & NTLMSSP_NEGOTIATE_KEY_EXCH)
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
    *pfNegotiateFlags = context->NegFlg;

    context->Credential = cred;
    //*ptsExpiry = 
    *pNewContext = context;

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
    PNTLMSSP_CONTEXT_CLI newContext = NULL;
    ULONG NegotiateFlags;

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

        /* new context is referenced! */
        ret = CliCreateContext(phCredential->dwLower,
                               pszTargetName,
                               fContextReq,
                               &newContext,
                               pfContextAttr,
                               ptsExpiry,
                               &NegotiateFlags);

        if(!newContext || !NT_SUCCESS(ret))
        {
            ERR("NtlmCreateNegoContext failed with %lx\n", ret);
            goto fail;
        }

        ret = CliGenerateNegotiateMessage(newContext,
                                          fContextReq,
                                          OutputToken1);
        /* set result */
        phNewContext->dwUpper = NegotiateFlags;
        phNewContext->dwLower = (ULONG_PTR)newContext;

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
        ret = CliGenerateAuthenticationMessage(
            phNewContext->dwLower, fContextReq,
            InputToken1, InputToken2,
            OutputToken1, OutputToken2,
            pfContextAttr, ptsExpiry, &NegotiateFlags);
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
        NtlmDereferenceContext((ULONG_PTR)newContext);

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
QueryContextAttributesAW(
    IN PCtxtHandle phContext,
    IN ULONG ulAttribute,
    OUT void *pBuffer,
    IN BOOL isUnicode)
{
    const WCHAR* PKG_NAME_W = L"NTLM";
    const WCHAR* PKG_COMMENT_W = L"NTLM Security Package";
    const char* PKG_NAME_A = "NTLM";
    const char* PKG_COMMENT_A = "NTLM Security Package";

    SECURITY_STATUS ret = SEC_E_OK;
    PNTLMSSP_CONTEXT_HDR context = NtlmReferenceContextHdr(phContext->dwLower);

    TRACE("%p %lx %p\n", phContext, ulAttribute, pBuffer);

    if (!context)
        return SEC_E_INVALID_HANDLE;

    switch(ulAttribute)
    {
        case SECPKG_ATTR_SIZES:
        {
            PSecPkgContext_Sizes spcs  = (PSecPkgContext_Sizes) pBuffer;
            spcs->cbMaxToken = NTLM_MAX_BUF;
            spcs->cbMaxSignature = sizeof(NTLMSSP_MESSAGE_SIGNATURE);
            spcs->cbBlockSize = 0;
            spcs->cbSecurityTrailer = sizeof(NTLMSSP_MESSAGE_SIGNATURE);
            break;
        }
        case SECPKG_ATTR_FLAGS:
        {
            ULONG negoFlags;
            PSecPkgContext_Flags spcf = (PSecPkgContext_Flags)pBuffer;
            spcf->Flags = 0;
            if (context->isServer)
                negoFlags = ((PNTLMSSP_CONTEXT_SVR)context)->CfgFlg;
            else
                negoFlags = ((PNTLMSSP_CONTEXT_CLI)context)->NegFlg;
            if (negoFlags & NTLMSSP_NEGOTIATE_SIGN)
                spcf->Flags |= ISC_RET_INTEGRITY;
            if (negoFlags & NTLMSSP_NEGOTIATE_SEAL)
                spcf->Flags |= ISC_RET_CONFIDENTIALITY;
            break;
        }
        case SECPKG_ATTR_NEGOTIATION_INFO:
        {
            PBYTE pOffset;
            ULONG spiSize;
            /* windows reports the same */
            /* I did some test, data of name and comment
             * point to data after the struct. So i assume
             * its one memory block. */
            if (isUnicode)
            {
                PSecPkgContext_NegotiationInfoW spcniW = (PSecPkgContext_NegotiationInfoW)pBuffer;
                PSecPkgInfoW spiW = (PSecPkgInfoW)pBuffer;
                PBYTE pOffset;

                spiSize = sizeof(SecPkgInfoW) +
                          (wcslen(PKG_NAME_W) + 1) * sizeof(WCHAR) +
                          (wcslen(PKG_COMMENT_W) + 1) * sizeof(WCHAR);

                spiW = (PSecPkgInfoW)NtlmAllocate(spiSize);
                spiW->fCapabilities = 0x82b37;
                spiW->cbMaxToken = 2888;
                spiW->wVersion = 1;
                spiW->wRPCID = 10;

                pOffset = (PBYTE)spiW + sizeof(SecPkgInfoW);
                NtlmStructWriteStrW(spiW, spiSize, &spiW->Name, PKG_NAME_W, &pOffset);
                NtlmStructWriteStrW(spiW, spiSize, &spiW->Comment, PKG_COMMENT_W, &pOffset);

                spcniW->NegotiationState = 0;
                spcniW->PackageInfo = spiW;
            }
            else
            {
                PSecPkgContext_NegotiationInfoA spcniA = (PSecPkgContext_NegotiationInfoA)pBuffer;
                PSecPkgInfoA spiA = (PSecPkgInfoA)pBuffer;

                spiSize = sizeof(SecPkgInfoA) +
                          (strlen(PKG_NAME_A) + 1) * sizeof(char) +
                          (strlen(PKG_COMMENT_A) + 1) * sizeof(char);

                spiA = (PSecPkgInfoA)NtlmAllocate(spiSize);
                spiA->fCapabilities = 0x82b37;
                spiA->cbMaxToken = 2888;
                spiA->wVersion = 1;
                spiA->wRPCID = 10;

                pOffset = (PBYTE)spiA + sizeof(SecPkgInfoA);
                NtlmStructWriteStrA(spiA, spiSize, &spiA->Name, PKG_NAME_A, &pOffset);
                NtlmStructWriteStrA(spiA, spiSize, &spiA->Comment, PKG_COMMENT_A, &pOffset);

                spcniA->NegotiationState = 0;
                spcniA->PackageInfo = spiA;
            }
            break;
        }
        case SECPKG_ATTR_PACKAGE_INFO:
        {
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
QueryContextAttributesW(
    IN PCtxtHandle phContext,
    IN ULONG ulAttribute,
    OUT void *pBuffer)
{
    return QueryContextAttributesAW(phContext, ulAttribute, pBuffer, TRUE);
}

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesA(
    IN PCtxtHandle phContext,
    IN ULONG ulAttribute,
    OUT void *pBuffer)
{
    return QueryContextAttributesAW(phContext, ulAttribute, pBuffer, FALSE);
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
        /* initialize with 0 to create a new context */
        phNewContext->dwLower = 0;
        TRACE("phNewContext->dwLower %lx\n", phNewContext->dwLower);
        ret = SvrHandleNegotiateMessage(phCredential->dwLower,
                                        &phNewContext->dwLower,
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
        ret = SvrHandleAuthenticateMessage(phNewContext->dwLower,
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

