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
#include "precomp.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/***********************************************************************
 *             EncryptMessage
 */
SECURITY_STATUS SEC_ENTRY
NtlmEncryptMessage(
    IN PNTLMSSP_CONTEXT_HDR Context,
    IN ULONG fQOP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    IN BOOL SignOnly)
{
    SECURITY_STATUS ret = SEC_E_OK;
    BOOL bRet;
    PSecBuffer data_buffer = NULL;
    PSecBuffer signature_buffer = NULL;
    prc4_key pSealHandle;
    PBYTE pSignKey;
    //PNTLMSSP_CONTEXT_MSG cli_msg;
    PULONG pSeqNum;
    ULONG index, cli_NegFlg;

    ERR("NtlmEncryptMessage(%p %d %p %d)\n", Context, fQOP, pMessage, MessageSeqNo);

    if(fQOP)
        FIXME("Ignoring fQOP\n");

    if (!Context)
        return SEC_E_INVALID_HANDLE;

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    /* get context, need to free it later! */
    /*cli_msg = */NtlmReferenceContextMsg((ULONG_PTR)Context, TRUE,
                                          &cli_NegFlg, &pSealHandle,
                                          &pSignKey, &pSeqNum);
    /*if (!ctxMsg->SendSealKey)
    {
        TRACE("context->SendSealKey is NULL\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }*/

    TRACE("pMessage->cBuffers %d\n", pMessage->cBuffers);
    /* extract data and signature buffers */
    for (index = 0; index < (int) pMessage->cBuffers; index++)
    {
        TRACE("pMessage->pBuffers[index].BufferType %d\n", pMessage->pBuffers[index].BufferType);
        if (pMessage->pBuffers[index].BufferType == SECBUFFER_DATA)
            data_buffer = &pMessage->pBuffers[index];
        else if (pMessage->pBuffers[index].BufferType == SECBUFFER_TOKEN)
            signature_buffer = &pMessage->pBuffers[index];
    }

    if (!data_buffer || !signature_buffer)
    {
        ERR("No data or tokens provided!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    if (signature_buffer->cbBuffer < sizeof(NTLMSSP_MESSAGE_SIGNATURE))
    {
        ret = SEC_E_BUFFER_TOO_SMALL;
        goto exit;
    }

    //printf("SealingKey (Client)\n");
    //NtlmPrintHexDump(cli_msg->ClientSealingKey, NTLM_SEALINGKEY_LENGTH);
    //printf("SealingKey (Server)\n");
    //NtlmPrintHexDump(cli_msg->ServerSealingKey, NTLM_SEALINGKEY_LENGTH);
    //printf("SigningKey (Client)\n");
    //NtlmPrintHexDump(cli_msg->ClientSigningKey, NTLM_SIGNKEY_LENGTH);
    //printf("SigningKey (Server)\n");
    //NtlmPrintHexDump(cli_msg->ServerSigningKey, NTLM_SIGNKEY_LENGTH);

    if (SignOnly)
        cli_NegFlg &= ~NTLMSSP_NEGOTIATE_SEAL;

    bRet = SEAL(cli_NegFlg, pSealHandle, (UCHAR*)pSignKey,
                NTLM_SIGNKEY_LENGTH, pSeqNum,
                data_buffer->pvBuffer, data_buffer->cbBuffer,
                signature_buffer->pvBuffer, &signature_buffer->cbBuffer);
    if (!bRet)
    {
        ret = SEC_E_INTERNAL_ERROR;
        goto exit;
    }

    //memcpy(signature_buffer->pvBuffer, (PBYTE)(data)+datalen-16, 16);
    //memcpy(data_buffer->pvBuffer, (PBYTE)(data), datalen-16);
    NtlmPrintHexDump(signature_buffer->pvBuffer, signature_buffer->cbBuffer);
    NtlmPrintHexDump(data_buffer->pvBuffer, data_buffer->cbBuffer);

exit:
    NtlmDereferenceContext((ULONG_PTR)Context);
    return ret;
}

/***********************************************************************
 *             DecryptMessage
 */
SECURITY_STATUS SEC_ENTRY
NtlmDecryptMessage(
    IN PNTLMSSP_CONTEXT_HDR Context,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQOP)
{
    SECURITY_STATUS ret = SEC_E_OK;
    BOOL bRet;
    PSecBuffer data_buffer = NULL;
    PSecBuffer signature_buffer = NULL;
    prc4_key pSealHandle;
    PBYTE pSignKey;
    //PNTLMSSP_CONTEXT_MSG cli_msg;
    PULONG pSeqNum;
    ULONG index, cli_NegFlg, expectedSignLen;
    NTLMSSP_MESSAGE_SIGNATURE expectedSign;

    ERR("NtlmDecryptMessage(%p %p %d)\n",
        Context, pMessage, MessageSeqNo);

    if (!Context)
        return SEC_E_INVALID_HANDLE;

    if (!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    /* get context, need to free it later! */
    /*cli_msg = */NtlmReferenceContextMsg((ULONG_PTR)Context, FALSE,
                                      &cli_NegFlg, &pSealHandle, &pSignKey, &pSeqNum);
    /*if (!ctxMsg->SendSealKey)
    {
        TRACE("context->SendSealKey is NULL\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }*/

    TRACE("pMessage->cBuffers %d\n", pMessage->cBuffers);
    /* extract data and signature buffers */
    for (index = 0; index < (int) pMessage->cBuffers; index++)
    {
        TRACE("pMessage->pBuffers[index].BufferType %d\n", pMessage->pBuffers[index].BufferType);
        if (pMessage->pBuffers[index].BufferType == SECBUFFER_DATA)
            data_buffer = &pMessage->pBuffers[index];
        else if (pMessage->pBuffers[index].BufferType == SECBUFFER_TOKEN)
            signature_buffer = &pMessage->pBuffers[index];
    }

    if (!data_buffer || !signature_buffer)
    {
        ERR("No data or tokens provided!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }

    if (signature_buffer->cbBuffer < sizeof(NTLMSSP_MESSAGE_SIGNATURE))
    {
        ret = SEC_E_BUFFER_TOO_SMALL;
        goto exit;
    }

    //printf("SealingKey (Client)\n");
    //NtlmPrintHexDump(cli_msg->ClientSealingKey, NTLM_SEALINGKEY_LENGTH);
    //printf("SealingKey (Server)\n");
    //NtlmPrintHexDump(cli_msg->ServerSealingKey, NTLM_SEALINGKEY_LENGTH);
    //printf("SigningKey (Client)\n");
    //NtlmPrintHexDump(cli_msg->ClientSigningKey, NTLM_SIGNKEY_LENGTH);
    //printf("SigningKey (Server)\n");
    //NtlmPrintHexDump(cli_msg->ServerSigningKey, NTLM_SIGNKEY_LENGTH);

    expectedSignLen = sizeof(expectedSign);
    bRet = UNSEAL(cli_NegFlg, pSealHandle, (UCHAR*)pSignKey,
                  NTLM_SIGNKEY_LENGTH, pSeqNum,
                  data_buffer->pvBuffer, data_buffer->cbBuffer,
                  (UCHAR*)&expectedSign, &expectedSignLen);
    if (!bRet)
    {
        ret = SEC_E_INTERNAL_ERROR;
        goto exit;
    }

    printf("sign ...\n");
    NtlmPrintHexDump((UCHAR*)&expectedSign, 16);
    NtlmPrintHexDump(signature_buffer->pvBuffer, 16);

    /* validate signature */
    if ((expectedSignLen != signature_buffer->cbBuffer) ||
        (memcmp(&expectedSign, signature_buffer->pvBuffer, expectedSignLen) != 0))
    {
        ret = SEC_E_MESSAGE_ALTERED;
        goto exit;
    }

    //memcpy(signature_buffer->pvBuffer, (PBYTE)(data)+datalen-16, 16);
    //memcpy(data_buffer->pvBuffer, (PBYTE)(data), datalen-16);
    NtlmPrintHexDump(signature_buffer->pvBuffer, signature_buffer->cbBuffer);
    NtlmPrintHexDump(data_buffer->pvBuffer, data_buffer->cbBuffer);

exit:
    NtlmDereferenceContext((ULONG_PTR)Context);
    return ret;
}
