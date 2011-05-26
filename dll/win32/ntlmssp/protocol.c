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

SECURITY_STATUS
NtlmGenerateNegotiateMessage(IN ULONG_PTR Context,
                             IN ULONG ContextReq,
                             IN PSecBuffer InputToken,
                             OUT PSecBuffer *OutputToken)
{
    PNTLMSSP_CONTEXT context = (PNTLMSSP_CONTEXT)Context;
    PNTLMSSP_CREDENTIAL cred = context->Credential;
    PNEGOTIATE_MESSAGE message;
    ULONG messageSize = 0;
    ULONG_PTR offset;
    NTLM_BLOB blobBuffer[2]; //nego contains 2 blobs

    if(!*OutputToken)
    {
        ERR("No output token!\n");
        return SEC_E_BUFFER_TOO_SMALL;
    }

    if(!((*OutputToken)->pvBuffer))
    {
        /* according to wine test */
        ERR("No output buffer!\n");
        return SEC_E_INTERNAL_ERROR;
    }

    messageSize = sizeof(NEGOTIATE_MESSAGE) +
                  NtlmOemComputerNameString.Length +
                  NtlmOemDomainNameString.Length;

    /* if should not allocate */
    if (!(ContextReq & ISC_REQ_ALLOCATE_MEMORY))
    {
        /* not enough space */
        if(messageSize > (*OutputToken)->cbBuffer)
            return SEC_E_BUFFER_TOO_SMALL;
    }
    else
    {
        /* allocate */
        (*OutputToken)->pvBuffer = NtlmAllocate(messageSize);
        (*OutputToken)->cbBuffer = messageSize;

        if(!(*OutputToken)->pvBuffer)
            return SEC_E_INSUFFICIENT_MEMORY;
    }

    /* allocate a negotiate message */
    message = (PNEGOTIATE_MESSAGE) NtlmAllocate(messageSize);

    if(!message)
        return SEC_E_INSUFFICIENT_MEMORY;

    /* build message */
    strcpy(message->Signature, NTLMSSP_SIGNATURE);
    message->MsgType = NtlmNegotiate;
    message->NegotiateFlags = context->NegotiateFlags;

    TRACE("nego message %p size %lu\n", message, messageSize);

    offset = (ULONG_PTR)(message+1);

    /* local connection */
    if((!cred->DomainName.Buffer && !cred->UserName.Buffer &&
        !cred->Password.Buffer) && cred->SecToken)
    {
        FIXME("try use local cached credentials?\n");

        message->NegotiateFlags |= (NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED |
            NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED | NTLMSSP_NEGOTIATE_LOCAL_CALL);

        NtlmUnicodeStringToBlob((PVOID)message,
                                (PUNICODE_STRING)&NtlmOemComputerNameString,
                                &message->OemWorkstationName,
                                &offset);
        NtlmUnicodeStringToBlob((PVOID)message,
                                (PUNICODE_STRING)&NtlmOemDomainNameString,
                                &message->OemDomainName,
                                &offset);
    }
    else
    {
        blobBuffer[0].Length = blobBuffer[0].MaxLength = 0;
        blobBuffer[0].Offset = offset;
        blobBuffer[1].Length =  blobBuffer[1].MaxLength = 0;
        blobBuffer[1].Offset = offset+1;
    }

    memset(&message->Version, 0, sizeof(NTLM_WINDOWS_VERSION));

    /* send it back */
    memcpy((*OutputToken)->pvBuffer, message, messageSize);
    (*OutputToken)->cbBuffer = messageSize;
    context->State = NegotiateSent;

    TRACE("context %p context->NegotiateFlags:\n",context);
    NtlmPrintNegotiateFlags(message->NegotiateFlags);

    /* free resources */
    NtlmFree(message);

    return SEC_I_CONTINUE_NEEDED;
}

SECURITY_STATUS
NtlmHandleNegotiateMessage(IN ULONG_PTR hCredential,
                           IN OUT PULONG_PTR phContext,
                           IN ULONG ContextReq,
                           IN PSecBuffer InputToken,
                           OUT PSecBuffer *pOutputToken,
                           OUT PULONG pContextAttr,
                           OUT PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PNEGOTIATE_MESSAGE negoMessage = NULL;
    PNTLMSSP_CREDENTIAL cred = NULL;
    PNTLMSSP_CONTEXT newContext = NULL;

    ERR("NtlmHandleNegotiateMessage called!\n");

    /* InputToken should contain a negotiate message*/
    if(InputToken->cbBuffer > NTLM_MAX_BUF ||
        InputToken->cbBuffer < sizeof(NEGOTIATE_MESSAGE))
    {
        ERR("Input token too big!!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    /* allocate a buffer for it */
    negoMessage = NtlmAllocate(InputToken->cbBuffer);

    if(!negoMessage)
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto exit;
    }

    /* copy it */
    memcpy(negoMessage, InputToken->pvBuffer, InputToken->cbBuffer);

    /* validate it */
    if(strncmp(negoMessage->Signature, NTLMSSP_SIGNATURE, 8) &&
       negoMessage->MsgType == NtlmNegotiate)
    {
        ERR("Input message not valid!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    TRACE("Got valid nego message! with flags:\n");
    NtlmPrintNegotiateFlags(negoMessage->NegotiateFlags);

    /* get credentials */
    cred = NtlmReferenceCredential(hCredential);
    if(!cred)
        goto exit;

    /* must be an incomming request */
    if(!(cred->UseFlags & SECPKG_CRED_INBOUND))
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        goto exit;
    }

    /* create new context */
    newContext = NtlmAllocateContext();
    if(!newContext)
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto exit;
    }

    *phContext = (ULONG_PTR)newContext;

    if(ContextReq & ASC_REQ_IDENTIFY)
    {
        *pContextAttr |= ASC_RET_IDENTIFY;
        newContext->ContextFlags |= ASC_RET_IDENTIFY;
    }

    if(ContextReq & ASC_REQ_DATAGRAM)
    {
        *pContextAttr |= ASC_RET_DATAGRAM;
        newContext->ContextFlags |= ASC_RET_DATAGRAM;
    }

    if(ContextReq & ASC_REQ_CONNECTION)
    {
        *pContextAttr |= ASC_RET_CONNECTION;
        newContext->ContextFlags |= ASC_RET_CONNECTION;
    }

    if(ContextReq & ASC_REQ_INTEGRITY)
    {
        *pContextAttr |= ASC_RET_INTEGRITY;
        newContext->ContextFlags |= ASC_RET_INTEGRITY;
    }

    if(ContextReq & ASC_REQ_REPLAY_DETECT)
    {
        *pContextAttr |= ASC_RET_REPLAY_DETECT;
        newContext->ContextFlags |= ASC_RET_REPLAY_DETECT;
    }

    if(ContextReq & ASC_REQ_SEQUENCE_DETECT)
    {
        *pContextAttr |= ASC_RET_SEQUENCE_DETECT;
        newContext->ContextFlags |= ASC_RET_SEQUENCE_DETECT;
    }

    if(ContextReq & ASC_REQ_ALLOW_NULL_SESSION)
    {
        newContext->ContextFlags |= ASC_REQ_ALLOW_NULL_SESSION;
    }

    if(ContextReq & ASC_REQ_ALLOW_NON_USER_LOGONS)
    {
        *pContextAttr |= ASC_RET_ALLOW_NON_USER_LOGONS;
        newContext->ContextFlags |= ASC_RET_ALLOW_NON_USER_LOGONS;
    }

    /* encryption */
    if(ContextReq & ASC_REQ_CONFIDENTIALITY)
    {
        *pContextAttr |= ASC_RET_CONFIDENTIALITY;
        newContext->ContextFlags |= ASC_RET_CONFIDENTIALITY;
    }

exit:
    ERR("FIXME: HERE!!!!!!!!!!!!!!!");
    return ret;
}

SECURITY_STATUS
NtlmHandleChallengeMessage(IN ULONG_PTR hContext,
                           IN ULONG ContextReq,
                           IN PSecBuffer InputToken1,
                           IN PSecBuffer InputToken2,
                           IN OUT PSecBuffer *OutputToken1,
                           IN OUT PSecBuffer *OutputToken2,
                           OUT PULONG pContextAttr,
                           OUT PTimeStamp ptsExpiry,
                           OUT PULONG NegotiateFlags)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PNTLMSSP_CONTEXT context;
    PCHALLENGE_MESSAGE challenge;
    BOOLEAN isUnicode;

    /* get context */
    context = NtlmReferenceContext(hContext);
    if(!context || !context->Credential)
    {
        ERR("NtlmHandleChallengeMessage invalid handle!\n");
        ret = SEC_E_INVALID_HANDLE;
        goto exit;
    }

    /* re-authenticate call */
    if(context->State == AuthenticateSent)
    {
        UNIMPLEMENTED;
    }
    else if(context->State != NegotiateSent)
    {
        ERR("Context not in negotiate sent state!\n");
        ret = SEC_E_OUT_OF_SEQUENCE;
        goto exit;
    }

    /* InputToken1 should contain a challenge message */
    TRACE("input token size %lx\n", InputToken1->cbBuffer);
    if(InputToken1->cbBuffer > NTLM_MAX_BUF ||
        InputToken1->cbBuffer < sizeof(CHALLENGE_MESSAGE))
    {
        ERR("Input token invalid!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    /* allocate a buffer for it */
    challenge = NtlmAllocate(InputToken1->cbBuffer);
    if(!challenge)
    {
        ERR("failed to allocate challenge buffer!\n");
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto exit;
    }

    /* copy it */
    memcpy(challenge, InputToken1->pvBuffer, InputToken1->cbBuffer);

    /* validate it */
    if(strncmp(challenge->Signature, NTLMSSP_SIGNATURE, 8) &&
       challenge->MsgType == NtlmChallenge)
    {
        ERR("Input message not valid!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    TRACE("Got valid challege message! with flags:\n");
    NtlmPrintNegotiateFlags(challenge->NegotiateFlags);

    /* print challenge message and payloads */
    NtlmPrintHexDump((PBYTE)challenge, InputToken1->cbBuffer);

    if(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM)
    {
        /* take out bad flags */
        challenge->NegotiateFlags &=
            (context->NegotiateFlags |
            NTLMSSP_NEGOTIATE_TARGET_INFO |
            NTLMSSP_TARGET_TYPE_SERVER |
            NTLMSSP_TARGET_TYPE_DOMAIN |
            NTLMSSP_NEGOTIATE_LOCAL_CALL);
    }

    if(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO)
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_TARGET_INFO;
    else
        context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_TARGET_INFO);

    /* if caller supports unicode prefer it over oem */
    if(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE)
    {
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
        context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_OEM;
        isUnicode = TRUE;
    }
    else if(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM)
    {
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
        context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_UNICODE;
        isUnicode = FALSE;
    }
    else
    {
        /* these flags must be bad! */
        ERR("challenge flags did not specify unicode or oem!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    /* support ntlm2 */
    if(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
    {
        challenge->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
        context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_LM_KEY);
    }
    else
    {
        /* did not support ntlm2 */
        context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY);

        /* did not support ntlm */
        if(!(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM))
        {
            ERR("netware authentication not supported!!!\n");
            ret = SEC_E_UNSUPPORTED_FUNCTION;
            goto exit;
        }
    }

    /* did not support 128bit encryption */
    if(!(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_128))
        context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_128);

    /* did not support 56bit encryption */
    if(!(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_56))
        context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_56);

    /* did not support lm key */
    if(!(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY))
        context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_LM_KEY);

    /* did not support key exchange */
    if(!(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH))
        context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_KEY_EXCH);

    /* should sign */
    if(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN)
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;
    else
        context->NegotiateFlags &= ~(NTLMSSP_NEGOTIATE_ALWAYS_SIGN);

    /* obligatory key exchange */
    if((context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM) &&
        (context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN | NTLMSSP_NEGOTIATE_SEAL)))
        context->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;

    /* unimplemented */
    if(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_LOCAL_CALL)
        ERR("NTLMSSP_NEGOTIATE_LOCAL_CALL set!\n");

    /* extract target info */
    UNICODE_STRING ServerName;

    if(context->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO)
    {
        /* wtf? shouldnt this contain AV pairs? */
        ret = NtlmBlobToUnicodeString(InputToken1,
                                      challenge->TargetInfo,
                                      &ServerName);
        if(!NT_SUCCESS(ret))
        {
            ERR("could not get target info!\n");
            goto exit;
        }
    }
    else
    {
        /* spec: "A server that is a member of a domain returns the domain of which it
         * is a member, and a server that is not a member of a domain returns
         * the server name." how to tell?? */
        ret = NtlmBlobToUnicodeString(InputToken1,
                                      challenge->TargetName,
                                      &ServerName);
        if(!NT_SUCCESS(ret))
        {
            ERR("could not get target info!\n");
            goto exit;
        }
        if(isUnicode)
            FIXME("convert to unicode!\n");
    }
    TRACE("ServerName %s\n", debugstr_w(ServerName.Buffer));

    PNTLMSSP_CREDENTIAL cred = NtlmReferenceCredential((ULONG_PTR)context->Credential);
    MSV1_0_NTLM3_RESPONSE NtResponse;
    LM2_RESPONSE Lm2Response;
    USER_SESSION_KEY UserSessionKey;
    LM_SESSION_KEY LmSessionKey;

    NtlmUnProtectMemory(cred->Password.Buffer, cred->Password.Length * sizeof(WCHAR));

    TRACE("cred: %s %s %s\n", debugstr_w(cred->UserName.Buffer),
        debugstr_w(cred->Password.Buffer), debugstr_w(cred->DomainName.Buffer));

    NtlmChallengeResponse(&cred->UserName,
                          &cred->Password,
                          &cred->DomainName,
                          &ServerName,
                          challenge->ServerChallenge,
                          &NtResponse,
                          &Lm2Response,
                          &UserSessionKey,
                          &LmSessionKey);

exit:
    ERR("handle challenge end\n");

    return ret;
}

