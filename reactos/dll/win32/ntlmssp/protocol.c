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
                             OUT PSecBuffer OutputToken)
{
    PNTLMSSP_CONTEXT context = (PNTLMSSP_CONTEXT)Context;
    PNTLMSSP_CREDENTIAL cred = context->Credential;
    PNEGOTIATE_MESSAGE message;
    ULONG messageSize = 0;
    ULONG_PTR offset;
    NTLM_BLOB blobBuffer[2]; //nego contains 2 blobs

    if(!OutputToken)
    {
        ERR("No output token!\n");
        return SEC_E_BUFFER_TOO_SMALL;
    }

    if(!(OutputToken->pvBuffer))
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
        if(messageSize > OutputToken->cbBuffer)
            return SEC_E_BUFFER_TOO_SMALL;
    }
    else
    {
        /* allocate */
        OutputToken->pvBuffer = NtlmAllocate(messageSize);
        OutputToken->cbBuffer = messageSize;

        if(!OutputToken->pvBuffer)
            return SEC_E_INSUFFICIENT_MEMORY;
    }

    /* use allocated memory */
    message = (PNEGOTIATE_MESSAGE) OutputToken->pvBuffer;
    offset = (ULONG_PTR)(message+1);

    /* build message */
    strncpy(message->Signature, NTLMSSP_SIGNATURE, sizeof(NTLMSSP_SIGNATURE));
    message->MsgType = NtlmNegotiate;
    message->NegotiateFlags = context->NegotiateFlags;

    TRACE("nego message %p size %lu\n", message, messageSize);
    TRACE("context %p context->NegotiateFlags:\n",context);
    NtlmPrintNegotiateFlags(message->NegotiateFlags);

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

    /* zero version struct */
    memset(&message->Version, 0, sizeof(NTLM_WINDOWS_VERSION));

    /* set state */
    context->State = NegotiateSent;
    return SEC_I_CONTINUE_NEEDED;
}

SECURITY_STATUS
NtlmGenerateChallengeMessage(IN PNTLMSSP_CONTEXT Context,
                             IN PNTLMSSP_CREDENTIAL Credentials,
                             IN UNICODE_STRING TargetName,
                             IN ULONG MessageFlags,
                             OUT PSecBuffer OutputToken)
{
    PCHALLENGE_MESSAGE chaMessage = NULL;
    ULONG messageSize, offset;

    /* compute message size */
    messageSize = sizeof(CHALLENGE_MESSAGE) + NtlmAvTargetInfo.Length;
    if(TargetName.Length > 0)
        messageSize += TargetName.Length;

    ERR("generating chaMessage of size %lu\n", messageSize);

    /*
     * according to tests ntlm does not listen to ASC_REQ_ALLOCATE_MEMORY
     * or lack thereof, furthermore the buffer size is always NTLM_MAX_BUF
     */
    OutputToken->pvBuffer = NtlmAllocate(NTLM_MAX_BUF);
    OutputToken->cbBuffer = messageSize;

    /* check allocation */
    if(!OutputToken->pvBuffer)
        return SEC_E_INSUFFICIENT_MEMORY;

    /* use allocated memory */
    chaMessage = (PCHALLENGE_MESSAGE)OutputToken->pvBuffer;

    /* build message */
    strncpy(chaMessage->Signature, NTLMSSP_SIGNATURE, sizeof(NTLMSSP_SIGNATURE));
    chaMessage->MsgType = NtlmChallenge;
    chaMessage->NegotiateFlags = Context->NegotiateFlags;
    chaMessage->NegotiateFlags |= NTLMSSP_NEGOTIATE_NTLM;

    /* generate server challenge */
    NtlmGenerateRandomBits(chaMessage->ServerChallenge, MSV1_0_CHALLENGE_LENGTH);
    memcpy(Context->Challenge, chaMessage->ServerChallenge, MSV1_0_CHALLENGE_LENGTH);

    /* point to the end of chaMessage */
    offset = (ULONG_PTR)(chaMessage+messageSize);

    /* set target information */
    ERR("set target information chaMessage %p to %S, len %d, offset %d\n", chaMessage, TargetName.Buffer, TargetName.Length, &offset);
    NtlmUnicodeStringToBlob((PVOID)chaMessage, &TargetName, &chaMessage->TargetName, &offset);

    //ERR("set target information %p to avl %S, %d, %d\n", chaMessage, NtlmAvTargetInfo.Buffer, NtlmAvTargetInfo.Length, offset);
    //NtlmPrintAvPairs(&NtlmAvTargetInfo);
    //NtlmUnicodeStringToBlob((PVOID)chaMessage, &NtlmAvTargetInfo, &chaMessage->TargetInfo, &offset);
    chaMessage->NegotiateFlags |= MessageFlags;

    /* set state */
    Context->State = ChallengeSent;
    return SEC_I_CONTINUE_NEEDED;
}

SECURITY_STATUS
NtlmHandleNegotiateMessage(IN ULONG_PTR hCredential,
                           IN OUT ULONG_PTR hContext,
                           IN ULONG ContextReq,
                           IN PSecBuffer InputToken,
                           IN PSecBuffer InputToken2,
                           OUT PSecBuffer OutputToken,
                           OUT PSecBuffer OutputToken2,
                           OUT PULONG pContextAttr,
                           OUT PTimeStamp ptsExpiry)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PNEGOTIATE_MESSAGE negoMessage = NULL;
    PNTLMSSP_CREDENTIAL cred = NULL;
    PNTLMSSP_CONTEXT context = NULL;
    UNICODE_STRING targetName;
    OEM_STRING OemDomainName, OemWorkstationName;
    ULONG negotiateFlags = 0;

    TRACE("NtlmHandleNegotiateMessage hContext %lx\n", hContext);
    if(!(context = NtlmAllocateContext()))
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        ERR("SEC_E_INSUFFICIENT_MEMORY!\n");
        goto exit;
    }

    hContext = (ULONG_PTR)context;

    /* InputToken should contain a negotiate message */
    if(InputToken->cbBuffer > NTLM_MAX_BUF ||
        InputToken->cbBuffer < sizeof(NEGOTIATE_MESSAGE))
    {
        ERR("Input wrong size!!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    /* allocate a buffer for it */
    if(!(negoMessage = NtlmAllocate(sizeof(NEGOTIATE_MESSAGE))))
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto exit;
    }

    /* copy it */
    memcpy(negoMessage, InputToken->pvBuffer, sizeof(NEGOTIATE_MESSAGE));

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
    if(!(cred = NtlmReferenceCredential(hCredential)))
    {
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    /* must be an incomming request */
    if(!(cred->UseFlags & SECPKG_CRED_INBOUND))
    {
        ret = SEC_E_UNSUPPORTED_FUNCTION;
        goto exit;
    }

    /* convert flags */
    if(ContextReq & ASC_REQ_IDENTIFY)
    {
        *pContextAttr |= ASC_RET_IDENTIFY;
        context->ContextFlags |= ASC_RET_IDENTIFY;
    }

    if(ContextReq & ASC_REQ_DATAGRAM)
    {
        *pContextAttr |= ASC_RET_DATAGRAM;
        context->ContextFlags |= ASC_RET_DATAGRAM;
    }

    if(ContextReq & ASC_REQ_CONNECTION)
    {
        *pContextAttr |= ASC_RET_CONNECTION;
        context->ContextFlags |= ASC_RET_CONNECTION;
    }

    if(ContextReq & ASC_REQ_INTEGRITY)
    {
        *pContextAttr |= ASC_RET_INTEGRITY;
        context->ContextFlags |= ASC_RET_INTEGRITY;
    }

    if(ContextReq & ASC_REQ_REPLAY_DETECT)
    {
        *pContextAttr |= ASC_RET_REPLAY_DETECT;
        context->ContextFlags |= ASC_RET_REPLAY_DETECT;
    }

    if(ContextReq & ASC_REQ_SEQUENCE_DETECT)
    {
        *pContextAttr |= ASC_RET_SEQUENCE_DETECT;
        context->ContextFlags |= ASC_RET_SEQUENCE_DETECT;
    }

    if(ContextReq & ASC_REQ_ALLOW_NULL_SESSION)
    {
        context->ContextFlags |= ASC_REQ_ALLOW_NULL_SESSION;
    }

    if(ContextReq & ASC_REQ_ALLOW_NON_USER_LOGONS)
    {
        *pContextAttr |= ASC_RET_ALLOW_NON_USER_LOGONS;
        context->ContextFlags |= ASC_RET_ALLOW_NON_USER_LOGONS;
    }

    /* encryption */
    if(ContextReq & ASC_REQ_CONFIDENTIALITY)
    {
        *pContextAttr |= ASC_RET_CONFIDENTIALITY;
        context->ContextFlags |= ASC_RET_CONFIDENTIALITY;
    }

    if (negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY ||
        negoMessage->NegotiateFlags & NTLMSSP_REQUEST_TARGET)
    {
        negotiateFlags |= NTLMSSP_TARGET_TYPE_SERVER | NTLMSSP_REQUEST_TARGET | NTLMSSP_NEGOTIATE_TARGET_INFO;

        if (negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE)
        {
            negotiateFlags |= NTLMSSP_NEGOTIATE_UNICODE;
            RtlInitUnicodeString(&targetName, NtlmComputerNameString.Buffer);
        }
        else if(negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM)
        {
            negotiateFlags |= NTLMSSP_NEGOTIATE_OEM;
            RtlInitUnicodeString(&targetName, (PWCHAR)&NtlmOemComputerNameString.Buffer);
        }
        else
        {
            ret = SEC_E_INVALID_TOKEN;
            ERR("flags invalid!\n");
            goto exit;
        }
    }

    /* check for local call */
    if((negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED) &&
        (negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED))
    {
        NtlmBlobToUnicodeString(InputToken, negoMessage->OemDomainName, (PUNICODE_STRING)&OemDomainName);
        NtlmBlobToUnicodeString(InputToken, negoMessage->OemWorkstationName, (PUNICODE_STRING)&OemWorkstationName);

        if (RtlEqualString(&OemWorkstationName, &NtlmOemComputerNameString, FALSE) &&
            RtlEqualString(&OemDomainName, &NtlmOemDomainNameString, FALSE))
        {
            TRACE("local negotiate detected!\n");
            negotiateFlags |= NTLMSSP_NEGOTIATE_LOCAL_CALL;
        }
    }

    /* compute negotiate message flags */
    if(negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
    {
        negoMessage->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
        negotiateFlags |= NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY;
    }
    else if(negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_LM_KEY)
    {
        negotiateFlags |= NTLMSSP_NEGOTIATE_LM_KEY;
    }

    if(negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_ALWAYS_SIGN)
        negotiateFlags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;

    if(negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SIGN)
    {
        *pContextAttr |= (ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT);
        context->ContextFlags |= (ASC_RET_SEQUENCE_DETECT | ASC_RET_REPLAY_DETECT);
        negotiateFlags |= NTLMSSP_NEGOTIATE_SIGN;
    }

    if (negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_SEAL)
    {
        *pContextAttr |= ASC_RET_CONFIDENTIALITY;
        context->ContextFlags |= ASC_RET_CONFIDENTIALITY;
        negotiateFlags |= NTLMSSP_NEGOTIATE_SEAL;
    }

    if(negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH)
        negotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;

    /* client requested encryption */
    if(negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_128)
        negotiateFlags |= NTLMSSP_NEGOTIATE_128;
    else if(negoMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_56)
        negotiateFlags |= NTLMSSP_NEGOTIATE_56;

    if(negoMessage->NegotiateFlags & NTLMSSP_REQUEST_INIT_RESP)
    {
        *pContextAttr |= ASC_RET_IDENTIFY;
        context->ContextFlags |= ASC_RET_IDENTIFY;
        negotiateFlags |= NTLMSSP_REQUEST_INIT_RESP;
    }

    ret = NtlmGenerateChallengeMessage(context,
                                       cred,
                                       targetName,
                                       negotiateFlags,
                                       OutputToken);

exit:
    if(negoMessage) NtlmFree(negoMessage);
    if(cred) NtlmDereferenceCredential((ULONG_PTR)cred);
    if(targetName.Buffer) NtlmFree(targetName.Buffer);
    if(OemDomainName.Buffer) NtlmFree(OemDomainName.Buffer);
    if(OemWorkstationName.Buffer) NtlmFree(OemWorkstationName.Buffer);

    return ret;
}

SECURITY_STATUS
NtlmHandleChallengeMessage(IN ULONG_PTR hContext,
                           IN ULONG ContextReq,
                           IN PSecBuffer InputToken1,
                           IN PSecBuffer InputToken2,
                           IN OUT PSecBuffer OutputToken1,
                           IN OUT PSecBuffer OutputToken2,
                           OUT PULONG pContextAttr,
                           OUT PTimeStamp ptsExpiry,
                           OUT PULONG NegotiateFlags)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PNTLMSSP_CONTEXT context = NULL;
    PCHALLENGE_MESSAGE challenge = NULL;
    PNTLMSSP_CREDENTIAL cred = NULL;
    BOOLEAN isUnicode;
    UNICODE_STRING ServerName;
    MSV1_0_NTLM3_RESPONSE NtResponse;
    LM2_RESPONSE Lm2Response;
    USER_SESSION_KEY UserSessionKey;
    LM_SESSION_KEY LmSessionKey;

    PAUTHENTICATE_MESSAGE authmessage = NULL;
    ULONG_PTR offset;
    UNICODE_STRING NtResponseString;
    UNICODE_STRING LmResponseString;
    UNICODE_STRING UserSessionKeyString;
    UNICODE_STRING LmSessionKeyString;
    ULONG messageSize;

    TRACE("NtlmHandleChallengeMessage hContext %lx\n", hContext);
    /* get context */
    context = NtlmReferenceContext(hContext);
    if(!context || !context->Credential)
    {
        ERR("NtlmHandleChallengeMessage invalid handle!\n");
        ret = SEC_E_INVALID_HANDLE;
        goto fail;
    }

    /* re-authenticate call */
    if(context->State == AuthenticateSent)
    {
        UNIMPLEMENTED;
        goto fail;
    }
    else if(context->State != NegotiateSent)
    {
        ERR("Context not in correct state!\n");
        ret = SEC_E_OUT_OF_SEQUENCE;
        goto fail;
    }

    /* InputToken1 should contain a challenge message */
    if(InputToken1->cbBuffer > NTLM_MAX_BUF ||
        InputToken1->cbBuffer < sizeof(CHALLENGE_MESSAGE))
    {
        ERR("Input token invalid!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    /* allocate a buffer for it */
    if(!(challenge = NtlmAllocate(sizeof(CHALLENGE_MESSAGE))))
    {
        ERR("failed to allocate challenge buffer!\n");
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto fail;
    }

    /* copy it */
    memcpy(challenge, InputToken1->pvBuffer, sizeof(CHALLENGE_MESSAGE));

    /* validate it */
    if(strncmp(challenge->Signature, NTLMSSP_SIGNATURE, 8) &&
       challenge->MsgType == NtlmChallenge)
    {
        ERR("Input message not valid!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    TRACE("Got valid challege message! with flags:\n");
    NtlmPrintNegotiateFlags(challenge->NegotiateFlags);

    /* print challenge message and payloads */
    NtlmPrintHexDump((PBYTE)InputToken1->pvBuffer, InputToken1->cbBuffer);

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
        goto fail;
    }

    /* support ntlm2 */
    if(challenge->NegotiateFlags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
    {
        challenge->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
        context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
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
            goto fail;
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
    if(context->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO)
    {
        ERR("NTLMSSP_NEGOTIATE_TARGET_INFO\n");
        //PMSV1_0_AV_PAIR pAvPair;
        //pAvPair = (PMSV1_0_AV_PAIR)challenge->TargetInfo;
        
        //PVOID ptr = ((PUCHAR)InputToken1->pvBuffer + challenge->TargetInfo.Offset);
        //NtlmPrintAvPairs(ptr);
        //NtlmPrintHexDump(InputToken1->pvBuffer, InputToken1->cbBuffer);

        //NtlmAvlGet(pAvPair, MsvAvNbDomainName, GetRespRequest->ServerName.Length)

        //if(!NT_SUCCESS(ret))
        //{
        //    ERR("could not get target info!\n");
        //    goto fail;
        //}
    }
    //else
    {
        /* spec: "A server that is a member of a domain returns the domain of which it
         * is a member, and a server that is not a member of a domain returns
         * the server name." how to tell?? */
        ret = NtlmBlobToUnicodeString(InputToken1,
                                      challenge->TargetInfo,
                                      //challenge->TargetName,
                                      &ServerName);
        if(!NT_SUCCESS(ret))
        {
            ERR("could not get target info!\n");
            goto fail;
        }
        if(isUnicode)
            FIXME("convert to unicode!\n");
    }

    if(!(cred = NtlmReferenceCredential((ULONG_PTR)context->Credential)))
        goto fail;

    /* unscramble password */
    NtlmUnProtectMemory(cred->Password.Buffer, cred->Password.Length);

    TRACE("cred: %s %s %s %s\n", debugstr_w(cred->UserName.Buffer),
        debugstr_w(cred->Password.Buffer), debugstr_w(cred->DomainName.Buffer),
        debugstr_w(ServerName.Buffer));

    NtlmChallengeResponse(&cred->UserName,
                          &cred->Password,
                          &cred->DomainName,
                          &ServerName,
                          challenge->ServerChallenge,
                          &NtResponse,
                          &Lm2Response,
                          &UserSessionKey,
                          &LmSessionKey);

#define InitString(str, input) str.MaximumLength = str.Length = sizeof(input); str.Buffer = (WCHAR*)&input
    InitString(NtResponseString, NtResponse);
    InitString(LmResponseString, Lm2Response);
    InitString(UserSessionKeyString, UserSessionKey);
    InitString(LmSessionKeyString, LmSessionKey);

    messageSize = sizeof(AUTHENTICATE_MESSAGE) + ServerName.Length +
        cred->UserName.Length + cred->DomainName.Length + NtResponseString.Length +
        UserSessionKeyString.Length + LmSessionKeyString.Length + LmResponseString.Length;

    if(!(authmessage = (PAUTHENTICATE_MESSAGE)NtlmAllocate(messageSize)))
    {
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto fail;
    }

    strncpy(authmessage->Signature, NTLMSSP_SIGNATURE, sizeof(NTLMSSP_SIGNATURE));
    authmessage->MsgType = NtlmAuthenticate;

    offset = (ULONG_PTR)(authmessage+1);

    NtlmUnicodeStringToBlob((PVOID)authmessage,
                            &cred->DomainName,
                            &authmessage->DomainName,
                            &offset);
    
    NtlmUnicodeStringToBlob((PVOID)authmessage,
                            &cred->UserName,
                            &authmessage->UserName,
                            &offset);

    NtlmUnicodeStringToBlob((PVOID)authmessage,
                            &ServerName,
                            &authmessage->WorkstationName,
                            &offset);

    if (context->NegotiateFlags & NTLMSSP_NEGOTIATE_TARGET_INFO)
    {
        NtlmUnicodeStringToBlob((PVOID)authmessage,
                                &LmResponseString,
                                &authmessage->LmChallengeResponse,
                                &offset);
    }

    NtlmUnicodeStringToBlob((PVOID)authmessage,
                            &NtResponseString,
                            &authmessage->NtChallengeResponse,
                            &offset);

    NtlmUnicodeStringToBlob((PVOID)authmessage,
                            &UserSessionKeyString,
                            &authmessage->EncryptedRandomSessionKey,
                            &offset);

    /* if should not allocate */
    if (!(ContextReq & ISC_REQ_ALLOCATE_MEMORY))
    {
        /* not enough space */
        if(messageSize > OutputToken1->cbBuffer)
        {
            ret = SEC_E_BUFFER_TOO_SMALL;
            goto fail;
        }
    }
    else
    {
        /* allocate */
        if(!(OutputToken1->pvBuffer = NtlmAllocate(messageSize)))
        {
            ret = SEC_E_INSUFFICIENT_MEMORY;
            goto fail;
        }
    }

    OutputToken1->cbBuffer = messageSize; /* says wine test */
    memcpy(OutputToken1->pvBuffer, authmessage, messageSize);
    context->State = AuthenticateSent;
    ret = SEC_E_OK;

fail:
    if(authmessage) NtlmFree(authmessage);
    if(context) NtlmDereferenceContext((ULONG_PTR)context);
    if(cred) NtlmDereferenceCredential((ULONG_PTR)cred);
    ERR("handle challenge end\n");
    return ret;
}

SECURITY_STATUS
NtlmHandleAuthenticateMessage(IN ULONG_PTR hContext,
                              IN ULONG ContextReq,
                              IN PSecBuffer InputToken,
                              OUT PSecBuffer OutputToken,
                              OUT PULONG pContextAttr,
                              OUT PTimeStamp ptsExpiry,
                              OUT PUCHAR pSessionKey,
                              OUT PULONG pfUserFlags)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PNTLMSSP_CONTEXT context = NULL;
    PAUTHENTICATE_MESSAGE authMessage = NULL;
    UNICODE_STRING LmChallengeResponse, NtChallengeResponse, SessionKey;
    UNICODE_STRING UserName, Workstation, DomainName;
    //BOOLEAN isUnicode;

    TRACE("NtlmHandleAuthenticateMessage hContext %x!\n", hContext);
    /* get context */
    if(!(context = NtlmReferenceContext(hContext)))
    {
        ret = SEC_E_INVALID_HANDLE;
        goto fail;
    }

    TRACE("context->State %d\n", context->State);
    if(context->State != ChallengeSent && context->State != Authenticated)
    {
        ERR("Context not in correct state!\n");
        ret = SEC_E_OUT_OF_SEQUENCE;
        goto fail;
    }

    /* re-authorize */
    if(context->State == Authenticated)
        UNIMPLEMENTED;

    /* InputToken1 should contain a authenticate message */
    TRACE("input token size %lx\n", InputToken->cbBuffer);
    if(InputToken->cbBuffer > NTLM_MAX_BUF ||
        InputToken->cbBuffer < sizeof(AUTHENTICATE_MESSAGE))
    {
        ERR("Input token invalid!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    /* allocate a buffer for it */
    if(!(authMessage = NtlmAllocate(sizeof(AUTHENTICATE_MESSAGE))))
    {
        ERR("failed to allocate authMessage buffer!\n");
        ret = SEC_E_INSUFFICIENT_MEMORY;
        goto fail;
    }

    /* copy it */
    memcpy(authMessage, InputToken->pvBuffer, sizeof(AUTHENTICATE_MESSAGE));

    /* validate it */
    if(strncmp(authMessage->Signature, NTLMSSP_SIGNATURE, 8) &&
       authMessage->MsgType == NtlmAuthenticate)
    {
        ERR("Input message not valid!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    /* datagram */
    if(context->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM)
    {
        /* context and message dont agree on connection type! */
        if(!(authMessage->NegotiateFlags & NTLMSSP_NEGOTIATE_DATAGRAM))
        {
            ret = SEC_E_INVALID_TOKEN;
            goto fail;
        }

        /* use message flags */
        context->NegotiateFlags = authMessage->NegotiateFlags;

        /* need a key */
        if(context->NegotiateFlags & (NTLMSSP_NEGOTIATE_SIGN | NTLMSSP_NEGOTIATE_SEAL))
            context->NegotiateFlags |= NTLMSSP_NEGOTIATE_KEY_EXCH;

        /* remove lm key */
        if (context->NegotiateFlags & NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY)
            context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LM_KEY;
    }

    /* supports unicode */
    if(context->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE)
    {
        context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_OEM;
        //isUnicode = TRUE;
    }
    else if(context->NegotiateFlags & NTLMSSP_NEGOTIATE_OEM)
    {
        context->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_UNICODE;
        //isUnicode = FALSE;
    }
    else
    {
        /* these flags must be bad! */
        ERR("authenticate flags did not specify unicode or oem!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    if(!NT_SUCCESS(NtlmBlobToUnicodeString(InputToken,
        authMessage->LmChallengeResponse, &LmChallengeResponse)))
    {
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    if(!NT_SUCCESS(NtlmBlobToUnicodeString(InputToken,
        authMessage->NtChallengeResponse, &NtChallengeResponse)))
    {
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    if(!NT_SUCCESS(NtlmBlobToUnicodeString(InputToken,
        authMessage->UserName, &UserName)))
    {
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    if(!NT_SUCCESS(NtlmBlobToUnicodeString(InputToken,
        authMessage->WorkstationName, &Workstation)))
    {
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    if(!NT_SUCCESS(NtlmBlobToUnicodeString(InputToken,
        authMessage->DomainName, &DomainName)))
    {
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    //if(NTLMSSP_NEGOTIATE_KEY_EXCHANGE)
    if(!NT_SUCCESS(NtlmBlobToUnicodeString(InputToken,
        authMessage->EncryptedRandomSessionKey, &SessionKey)))
    {
        ret = SEC_E_INVALID_TOKEN;
        goto fail;
    }

    ret = SEC_I_COMPLETE_NEEDED;

fail:
    NtlmDereferenceContext((ULONG_PTR)context);
    return ret;
}
