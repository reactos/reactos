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

#include <precomp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

// lsa mode context
CRITICAL_SECTION ContextCritSect;
LIST_ENTRY ValidContextList;

SECURITY_STATUS
NtlmContextInitialize(VOID)
{
    __wine_dbch_ntlm.flags = 0x1;

    InitializeCriticalSection(&ContextCritSect);
    InitializeListHead(&ValidContextList);

    return SEC_E_OK;
}

PNTLMSSP_CONTEXT_HDR
NtlmReferenceContextHdr(
    IN LSA_SEC_HANDLE ContextHandle)
{
    PNTLMSSP_CONTEXT_HDR context;
    EnterCriticalSection(&ContextCritSect);

    context = (PNTLMSSP_CONTEXT_HDR)ContextHandle;

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

VOID
NtlmGetContextMsgKeys(
    IN PNTLMSSP_CONTEXT_HDR Context,
    IN BOOL isSending,
    OUT PULONG pNegFlg,
    OUT prc4_key* pSealHandle,
    OUT PBYTE* pSignKey,
    OUT PULONG* pSeqNum)
{
    if (Context->isServer)
    {
        PNTLMSSP_CONTEXT_SVR csvr = (PNTLMSSP_CONTEXT_SVR)Context;
        if (pNegFlg)
            *pNegFlg = csvr->cli_NegFlg;
        if (isSending)
        {
            *pSealHandle = &csvr->cli_msg.ServerHandle;
            *pSignKey    = (PBYTE)&csvr->cli_msg.ServerSigningKey;
            *pSeqNum     = &csvr->cli_msg.ServerSeqNum;
        }
        else
        {
            *pSealHandle = &csvr->cli_msg.ClientHandle;
            *pSignKey    = (PBYTE)&csvr->cli_msg.ClientSigningKey;
            *pSeqNum     = &csvr->cli_msg.ClientSeqNum;
        }
    }
    else
    {
        PNTLMSSP_CONTEXT_CLI ccli = (PNTLMSSP_CONTEXT_CLI)Context;
        if (pNegFlg)
            *pNegFlg = ccli->NegFlg;
        if (isSending)
        {
            *pSealHandle = &ccli->msg.ClientHandle;
            *pSignKey    = (PBYTE)&ccli->msg.ClientSigningKey;
            *pSeqNum     = &ccli->msg.ClientSeqNum;
        }
        else
        {
            *pSealHandle = &ccli->msg.ServerHandle;
            *pSignKey    = (PBYTE)&ccli->msg.ServerSigningKey;
            *pSeqNum     = &ccli->msg.ServerSeqNum;
        }
    }
}


VOID
NtlmDereferenceContext(
    IN PNTLMSSP_CONTEXT_HDR Context)
{
    EnterCriticalSection(&ContextCritSect);

    /* sanity */
    ASSERT(Context);
    TRACE("%p refcount %lu\n", Context, Context->RefCount);
    ASSERT(Context->RefCount >= 1);

    /* decrement reference */
    Context->RefCount--;

    /* check for object rundown */
    if (Context->RefCount == 0)
    {
        TRACE("Deleting context %p\n", Context);

        /* dereference credential */
        if (Context->isServer)
        {
            PNTLMSSP_CONTEXT_SVR csvr = (PNTLMSSP_CONTEXT_SVR)Context;
            if(csvr->Credential)
                NtlmDereferenceCredential((ULONG_PTR)csvr->Credential);
        }
        else
        {
            PNTLMSSP_CONTEXT_CLI ccli = (PNTLMSSP_CONTEXT_CLI)Context;
            if(ccli->Credential)
                NtlmDereferenceCredential((ULONG_PTR)ccli->Credential);
        }

        /* remove from list */
        RemoveEntryList(&Context->Entry);

        /* delete object */
        NtlmFree(Context, FALSE);
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

        NtlmDereferenceContext(Context);
    }

    LeaveCriticalSection(&ContextCritSect);

    /* free critical section */
    DeleteCriticalSection(&ContextCritSect);

    return;
}

PNTLMSSP_CONTEXT_HDR
NtlmAllocateContextHdr(
    IN BOOL isServer)
{
    SECPKG_CALL_INFO CallInfo;
    PNTLMSSP_CONTEXT_HDR ret;

    ULONG ctxSize = (isServer) ? sizeof(NTLMSSP_CONTEXT_SVR) :
                                 sizeof(NTLMSSP_CONTEXT_CLI);

    ret = (PNTLMSSP_CONTEXT_HDR)NtlmAllocate(ctxSize, FALSE);

    if(!ret)
    {
        ERR("allocate context failed!\n");
        return NULL;
    }

    /* set process fields */
    ret->ProcId = GetCurrentProcessId();

    ret->isServer = isServer;

    if(inLsaMode)
        if(LsaFunctions->GetCallInfo(&CallInfo))
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
    return ret;
}

SECURITY_STATUS
CliCreateContext(
    IN ULONG_PTR Credential,
    IN SEC_WCHAR *pszTargetName,
    IN ULONG ISCContextReq,
    OUT PNTLMSSP_CONTEXT_CLI* pNewContext,
    OUT PULONG pISCContextAttr,
    OUT PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PNTLMSSP_CONTEXT_CLI context = NULL;
    PNTLMSSP_CREDENTIAL cred;
    PNTLMSSP_GLOBALS_CLI gcli = getGlobalsCli();

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
    if (gcli->CliLMLevel & CLI_LMFLAG_USE_AUTH_NTLMv2)
        context->UseNTLMv2 = TRUE;
    else
        context->UseNTLMv2 = FALSE;

    /* always on features - MS-NLMP 3.1.5.1.1 */
    context->NegFlg = NTLMSSP_REQUEST_TARGET |
                      NTLMSSP_NEGOTIATE_ALWAYS_SIGN |
                      NTLMSSP_NEGOTIATE_UNICODE;

    /* if LM auth is not used set ext. session security (ntlmv2 session security) */
    if (gcli->CliLMLevel & (~CLI_LMFLAG_USE_AUTH_LM))
        context->NegFlg |= NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;
    if ((gcli->CliLMLevel & CLI_LMFLAG_USE_AUTH_NTLMv1) ||
        (gcli->CliLMLevel & CLI_LMFLAG_USE_AUTH_NTLMv2))
        context->NegFlg |= NTLMSSP_NEGOTIATE_NTLM;

    /* addiditonal flags / features w2k3 returns */
    context->NegFlg = context->NegFlg |
                      NTLMSSP_NEGOTIATE_SEAL |
                      NTLMSSP_NEGOTIATE_56 |
                      NTLMSSP_NEGOTIATE_128 |
                      NTLMSSP_NEGOTIATE_OEM |
                      NTLMSSP_NEGOTIATE_SIGN |
                      NTLMSSP_NEGOTIATE_VERSION |
                      NTLMSSP_NEGOTIATE_KEY_EXCH;

    /* client requested features */
    if (ISCContextReq & ISC_REQ_INTEGRITY)
    {
        *pISCContextAttr |= ISC_RET_INTEGRITY;
        context->NegFlg |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if (ISCContextReq & ISC_REQ_SEQUENCE_DETECT)
    {
        *pISCContextAttr |= ISC_RET_SEQUENCE_DETECT;
        context->NegFlg |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if (ISCContextReq & ISC_REQ_REPLAY_DETECT)
    {
        *pISCContextAttr |= ISC_RET_REPLAY_DETECT;
        context->NegFlg |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if (ISCContextReq & ISC_REQ_CONFIDENTIALITY)
    {
        context->NegFlg |= NTLMSSP_NEGOTIATE_SEAL |
                           NTLMSSP_NEGOTIATE_LM_KEY |
                           NTLMSSP_NEGOTIATE_KEY_EXCH |
                           NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;

        *pISCContextAttr |= ISC_RET_CONFIDENTIALITY;
    }

    if (ISCContextReq & ISC_REQ_NULL_SESSION)
    {
        *pISCContextAttr |= ISC_RET_NULL_SESSION;
    }

    if (ISCContextReq & ISC_REQ_CONNECTION)
    {
        *pISCContextAttr |= ISC_RET_CONNECTION;
    }

    if (ISCContextReq & ISC_REQ_IDENTIFY)
    {
        context->NegFlg |= NTLMSSP_REQUEST_INIT_RESP;
        *pISCContextAttr |= ISC_RET_IDENTIFY;
    }

    if (ISCContextReq & ISC_REQ_DATAGRAM)
    {
        /* datagram flags */
        context->NegFlg |= NTLMSSP_NEGOTIATE_DATAGRAM;
        context->NegFlg &= ~NTLMSSP_NEGOTIATE_NT_ONLY;
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

    /* Remove flags we dont support
     * Do not remove LM-KEY flag if EXTENDES_SECURITY exists.
     * => This is done in authentication message.
     */
    ValidateNegFlg(gcli->ClientConfigFlags, &context->NegFlg, TRUE, FALSE);

    context->Credential = cred;

    if (ptsExpiry)
        *ptsExpiry = cred->ExpirationTime;

    *pNewContext = context;

    return ret;

fail:
    /* free resources */
    NtlmDereferenceContext(&context->hdr);
    return ret;
}

/**
 * @brief Map a security context created in lsa mode for
          for user mode. This function is called by
          InitLsaModeContext and AcceptLsaModeContext.
          A mapped context is only needed if the flag
          DATAGRAM or LOCAL_CALL (ContextAttribute) is
          set.
 * @param Context the context
 * @param ContextAttr value of ContextAttribute that will
 *        be returned from Init/AcceptLsaModeContext
 * @param MappedContext will be set to true if a mapped
 *        context is returned.
 * @param Context Mapped context.
 */
SECURITY_STATUS
MapSecurityContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN OUT PSecBuffer MappedContext)
{
    PNTLMSSP_CONTEXT_HDR ContextHdr;
    ULONG ContextSize;

    // * This is not really what windows is doing.
    //   It's very unsecure to return our "internal" key
    //   states (i think).
    // * But my target for now is to get it work.
    // * Windows generates a "packed context". I need to
    //   investigate more what's in there. I guess windows
    //   computes the internal context from these values to
    //   avoid giving out to much internals (crypt-key-states
    //   and so on)
    ContextHdr = NtlmReferenceContextHdr(ContextHandle);
    if (ContextHdr->isServer)
        ContextSize = sizeof(NTLMSSP_CONTEXT_SVR);
    else
        ContextSize = sizeof(NTLMSSP_CONTEXT_CLI);

    MappedContext->cbBuffer = ContextSize;//0x29a;
    MappedContext->BufferType = SECBUFFER_EMPTY;
    MappedContext->pvBuffer = NtlmAllocate(MappedContext->cbBuffer, FALSE);

    RtlCopyMemory(MappedContext->pvBuffer, ContextHdr, ContextSize);

    NtlmDereferenceContext(ContextHdr);

    return SEC_E_OK;
}

/* public functions */

SECURITY_STATUS
SEC_ENTRY
NtlmInitializeSecurityContext(
    IN OPTIONAL LSA_SEC_HANDLE hCredential,
    IN OPTIONAL LSA_SEC_HANDLE hContext,
    IN OPTIONAL SEC_WCHAR *pszTargetName,
    IN ULONG fContextReq,
    IN ULONG Reserved1,
    IN ULONG TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN ULONG Reserved2,
    IN OUT OPTIONAL PLSA_SEC_HANDLE phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT ULONG *pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData)
{
    SECURITY_STATUS ret = SEC_E_OK;
    SECURITY_STATUS RetTmp;
    PSecBuffer InputToken1, InputToken2 = NULL;
    PSecBuffer OutputToken1, OutputToken2 = NULL;
    PNTLMSSP_CONTEXT_CLI newContext = NULL;

    TRACE("0x%lx 0x%lx %s 0x%08lx %lx %lx %p %lx %p %p %p %p\n", hCredential, hContext,
     debugstr_w(pszTargetName), fContextReq, Reserved1, TargetDataRep, pInput,
     Reserved1, phNewContext, pOutput, pfContextAttr, ptsExpiry);

    *MappedContext = FALSE;

    if(TargetDataRep == SECURITY_NETWORK_DREP)
        WARN("SECURITY_NETWORK_DREP!!\n");

    // get first input token
    if (!NtlmGetSecBufferType(pInput, SECBUFFER_TOKEN, 0,
                              FALSE, &InputToken1))
    {
        // should be ok? -> yes -> remove warning
        WARN("Failed to get input token!\n");
        InputToken1 = NULL;
        //return SEC_E_INVALID_TOKEN;
    }

    // get first output token;
    if (!NtlmGetSecBufferType(pOutput, SECBUFFER_TOKEN, 0,
                              TRUE, &OutputToken1))
    {
        ERR("Failed to get output token!\n");
        return SEC_E_BUFFER_TOO_SMALL;
    }

    /* first call! nego message creation */
    if ((hContext == 0) && (InputToken1 == NULL))
    {
        if(!hCredential)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto fail;
        }

        /* new context is referenced! */
        RetTmp = CliCreateContext(hCredential,//phCredential->dwLower,
                                  pszTargetName,
                                  fContextReq,
                                  &newContext,
                                  pfContextAttr,
                                  ptsExpiry);
        if (!newContext || !NT_SUCCESS(RetTmp))
        {
            ERR("NtlmCreateNegoContext failed with %lx\n", RetTmp);
            ret = RetTmp;
            goto fail;
        }

        ret = CliGenerateNegotiateMessage(newContext,
                                          fContextReq,
                                          OutputToken1);

        // set result
        *phNewContext = (LSA_SEC_HANDLE)newContext;

    }
    else if (hContext != 0)       /* challenge! */
    {
        TRACE("ISC challenged!\n");
        if (fContextReq & ISC_REQ_USE_SUPPLIED_CREDS)
        {
            /* get second input token */
            RetTmp = NtlmGetSecBuffer(pInput,
                                  1,
                                  &InputToken2,
                                  FALSE);
            if (!RetTmp)
            {
                ERR("Failed to get input token!\n");
                return SEC_E_INVALID_TOKEN;
            }
        }
        *phNewContext = hContext;
        ret = CliGenerateAuthenticationMessage(
            hContext, fContextReq,
            InputToken1, InputToken2,
            OutputToken1, OutputToken2,
            pfContextAttr, ptsExpiry);
    }
    else
    {
        ERR("bad call to InitializeSecurityContext\n");
        ret = SEC_E_INVALID_HANDLE;
        goto fail;
    }

    if (!NT_SUCCESS(ret))
    {
        ERR("failed with %lx\n", ret);
        __debugbreak();
        goto fail;
    }

    if(fContextReq & ISC_REQ_ALLOCATE_MEMORY)
        *pfContextAttr |= ISC_RET_ALLOCATED_MEMORY;

    if ( ((ret == SEC_I_CONTINUE_NEEDED) && ((*pfContextAttr) & ~ISC_RET_DATAGRAM)) ||
         ((ret == SEC_E_OK) && ((*pfContextAttr) & ~0x08000000)) )
    {
        TRACE("Mapping security context!");
        RetTmp = MapSecurityContext(*phNewContext, ContextData);
        if (RetTmp != SEC_E_OK)
        {
            ERR("MapSecurityContext failed\n");
            ret = RetTmp;
            goto fail;
        }
        *MappedContext = TRUE;
    }

    return ret;

fail:
    /* free resources */
    if(newContext)
        NtlmDereferenceContext(&newContext->hdr);

    if(fContextReq & ISC_REQ_ALLOCATE_MEMORY)
    {
        if(OutputToken1 && OutputToken1->pvBuffer)
            NtlmFree(OutputToken1->pvBuffer, FALSE);
        if(OutputToken2 && OutputToken2->pvBuffer)
            NtlmFree(OutputToken2->pvBuffer, FALSE);
    }

    return ret;
}

#ifdef __UNUSED__
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
#endif

SECURITY_STATUS
SEC_ENTRY
NtlmQueryContextAttributes(
    IN PNTLMSSP_CONTEXT_HDR Context,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer,
    IN BOOL isUnicode)
{
    const WCHAR* PKG_NAME_W = L"NTLM";
    const WCHAR* PKG_COMMENT_W = L"NTLM Security Package";
    const char* PKG_NAME_A = "NTLM";
    const char* PKG_COMMENT_A = "NTLM Security Package";

    SECURITY_STATUS ret = SEC_E_OK;

    TRACE("%p %lx %p\n", Context, ulAttribute, pBuffer);

    if (!Context)
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
            if (Context->isServer)
                negoFlags = ((PNTLMSSP_CONTEXT_SVR)Context)->cli_NegFlg;
            else
                negoFlags = ((PNTLMSSP_CONTEXT_CLI)Context)->NegFlg;
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

                spiW = (PSecPkgInfoW)NtlmAllocate(spiSize, FALSE);
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

                spiA = (PSecPkgInfoA)NtlmAllocate(spiSize, FALSE);
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

    return ret;
}

#ifdef __UNUSED__
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
#endif

SECURITY_STATUS
SEC_ENTRY
NtlmAcceptSecurityContext(
    IN LSA_SEC_HANDLE hCredential,
    IN LSA_SEC_HANDLE hContext,
    IN PSecBufferDesc pInput,
    IN ULONG fContextReq,
    IN ULONG TargetDataRep,
    IN OUT PLSA_SEC_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT ULONG *pfContextAttr,
    OUT PTimeStamp ptsExpiry,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData)
{
    SECURITY_STATUS ret = SEC_E_OK;
    BOOL RetTmp;
    PSecBuffer InputToken1, InputToken2 = NULL;
    PSecBuffer OutputToken1, OutputToken2 = NULL;
    USER_SESSION_KEY sessionKey;
    ULONG userflags;

    TRACE("AcceptSecurityContext %lx %lx %p %lx %lx %p %p %p %p\n",
        hCredential, hContext, pInput, fContextReq, TargetDataRep,
        phNewContext, pOutput, pfContextAttr, ptsExpiry);

    *MappedContext = FALSE;

    // get first input token
    if (!NtlmGetSecBufferType(pInput, SECBUFFER_TOKEN, 0,
                              FALSE, &InputToken1))
    {
        ERR("Failed to get input token!\n");
        return SEC_E_INVALID_TOKEN;
    }

    // get second input token
    if (!NtlmGetSecBufferType(pInput, SECBUFFER_TOKEN, 1,
                              FALSE, &InputToken2))
    {
        ERR("Failed to get input token 2!\n");
        //return SEC_E_INVALID_TOKEN;
    }

    // get first output token
    if (!NtlmGetSecBufferType(pOutput, SECBUFFER_TOKEN, 0,
                              TRUE, &OutputToken1))
    {
        ERR("Failed to get output token!\n");
        return SEC_E_BUFFER_TOO_SMALL;
    }

    // first call
    if (hContext == 0)// && !InputToken2)
    {
        if (hCredential == 0)
        {
            ret = SEC_E_INVALID_HANDLE;
            goto fail;
        }
        // initialize with 0 to create a new context
        *phNewContext = 0;
        TRACE("phNewContext->dwLower %lx\n", *phNewContext);
        ret = SvrHandleNegotiateMessage(hCredential,
                                        phNewContext,
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
        *phNewContext = hContext;
        TRACE("phNewContext->dwLower %lx\n", *phNewContext);
        ret = SvrHandleAuthenticateMessage((ULONG_PTR)*phNewContext,
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

    if(fContextReq & ASC_REQ_ALLOCATE_MEMORY)
        *pfContextAttr |= ASC_RET_ALLOCATED_MEMORY;

    if ((ret != SEC_E_OK) && ((*pfContextAttr) & ~0x08000000))
    {
        TRACE("Mapping security context!\n");
        RetTmp = MapSecurityContext(*phNewContext, ContextData);
        if (RetTmp != SEC_E_OK)
        {
            ERR("MapSecurityContext failed\n");
            ret = RetTmp;
            goto fail;
        }
        *MappedContext = TRUE;
    }

    return ret;
fail:
    return ret;
}

#ifdef __UNUSED__
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

#endif
