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
#include "ciphers.h"
#include "protocol.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

/***********************************************************************
 *             EncryptMessage
 */
SECURITY_STATUS SEC_ENTRY EncryptMessage(PCtxtHandle phContext,
        ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
    SECURITY_STATUS ret = SEC_E_OK;
    PSecBuffer data_buffer = NULL;
    PSecBuffer signature_buffer = NULL;
    //prc4_key pSendHandle, pRecvHandle;
    //UCHAR digest[16], checksum[8], *signature;
    UINT index, length;//, version = 1;
    //HMAC_MD5_CTX hmac;
    //PNTLMSSP_CONTEXT_MSG cli_msg;
    ULONG cli_NegFlg;

    void* data;

    ERR("EncryptMessage(%p %d %p %d)\n", phContext, fQOP, pMessage, MessageSeqNo);

    if(fQOP)
        FIXME("Ignoring fQOP\n");

    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    /* get context, need to free it later! */
    //cli_msg =  NtlmReferenceContextMsg(phContext->dwLower, &cli_NegFlg);
    cli_NegFlg = 0;//HACK
    /* not sure ... swap if we are server ?! */
    //pSendHandle = &cli_msg->ClientHandle;
    //pRecvHandle = &cli_msg->ServerHandle;

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

    /* Copy original data buffer */
    length = data_buffer->cbBuffer;
    data = NtlmAllocate(length);
    memcpy(data, data_buffer->pvBuffer, length);

    /* Compute the HMAC-MD5 hash of ConcatenationOf(seq_num,data) using the client signing key */
    /*HMACMD5Init(&hmac, ctxMsg->ClientSigningKey, sizeof(ctxMsg->ServerSigningKey));
    HMACMD5Update(&hmac, (void*) &(MessageSeqNo), sizeof(MessageSeqNo));
    HMACMD5Update(&hmac, data, length);
    HMACMD5Final(&hmac, digest);*/
    if (NTLMSSP_NEGOTIATE_SIGN & cli_NegFlg)
    {
        FIXME("SIGN not implemented\n");
        //Define SIGN(Handle, SigningKey, SeqNum, Message) as
        //ConcatenationOf(Message, MAC(Handle, SigningKey, SeqNum, Message))
        //EndDefine
        //SIGN
    }

    /* Encrypt message using with RC4, result overwrites original buffer */
    /*rc4_crypt(ctxMsg->SendSealKey, data_buffer->pvBuffer, data, length);
     * */
    FIXME("ENCRYPT\n");
    NtlmFree(data);
    /* RC4-encrypt first 8 bytes of digest */
    /*
    rc4_crypt(ctxMsg->SendSealKey, checksum, digest, 8);
    signature = (UCHAR*) signature_buffer->pvBuffer;
     */

    /* Concatenate version, ciphertext and sequence number to build signature */
    /*
    memcpy(signature, (void*) &version, sizeof(version));
    memcpy(&signature[4], (void*) checksum, sizeof(checksum));
    memcpy(&signature[12], (void*) &(MessageSeqNo), sizeof(MessageSeqNo));
    ctxMsg->SentSequenceNum++;
     */

exit:
    NtlmDereferenceContext(phContext->dwLower);
    return ret;
}

/***********************************************************************
 *             DecryptMessage
 */
SECURITY_STATUS SEC_ENTRY DecryptMessage(PCtxtHandle phContext,
        PSecBufferDesc pMessage, ULONG MessageSeqNo, PULONG pfQOP)
{
    SECURITY_STATUS ret = SEC_E_OK;
    ULONG index, length;
    //HMAC_MD5_CTX hmac;
    //UINT version = 1;
    //UCHAR digest[16], expected_signature[16], checksum[8];
    PSecBuffer data_buffer = NULL, signature = NULL;
    PVOID data;
    ULONG cli_NegFlg;
    prc4_key pRecvHandle, pSendHandle;
    PNTLMSSP_CONTEXT_MSG cli_msg;

    TRACE("(%p %p %d %p)\n", phContext, pMessage, MessageSeqNo, pfQOP);

    /* sanity checks */
    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    /* get context */
    cli_msg = NtlmReferenceContextMsg(phContext->dwLower, &cli_NegFlg, &pSendHandle, &pRecvHandle);

    /* extract data and signature buffers */
    for (index = 0; index < (int) pMessage->cBuffers; index++)
    {
        TRACE("buffer %i, type %i\n", index, pMessage->pBuffers[index].BufferType);
        NtlmPrintHexDump(pMessage->pBuffers[index].pvBuffer, pMessage->pBuffers[index].cbBuffer);
        if (pMessage->pBuffers[index].BufferType == SECBUFFER_DATA)
            data_buffer = &pMessage->pBuffers[index];
        else if (pMessage->pBuffers[index].BufferType == SECBUFFER_TOKEN)
            signature = &pMessage->pBuffers[index];
    }

    /* verify we got needed buffers */
    if (!data_buffer || !signature)
    {
        ERR("No data or tokens provided!\n");
        ret = SEC_E_INVALID_TOKEN;
        goto exit;
    }


    if ((cli_NegFlg & NTLMSSP_NEGOTIATE_SIGN) &&
        (signature->cbBuffer < 16))
    {
        ERR("Signature buffer too small!\n");
        ret = SEC_E_BUFFER_TOO_SMALL;
        goto exit;
    }
    
    /* Copy original data buffer */
    length = data_buffer->cbBuffer;
    data = NtlmAllocate(length);
    memcpy(data, data_buffer->pvBuffer, length);

    if (cli_NegFlg & NTLMSSP_NEGOTIATE_SEAL)
    {
        FIXME("Check SEAL\n");
        /* Decrypt message using with RC4 */
        FIXME("Decrypt (UNSEAL)\n");

        TRACE("RC4 ClientSealingKey\n");
        NtlmPrintHexDump(cli_msg->ClientSealingKey, NTLM_SEALINGKEY_LENGTH);
        TRACE("RC4 ServerSealingKey\n");
        NtlmPrintHexDump(cli_msg->ServerSealingKey, NTLM_SEALINGKEY_LENGTH);
        //rc4_crypt(ctxMsg->RecvSealKey, data_buffer->pvBuffer, data, length);
        /* TODO optimize ... RC4 uses in/out ... if
         * RC4 has in/out-buffer memcpy before could be removed */
        TRACE("RC4 before\n");
        NtlmPrintHexDump(data, length);
        RC4(pSendHandle, data, data, length);
    }
    TRACE("done\n");
    NtlmPrintHexDump(data, length);

    if (cli_NegFlg & NTLMSSP_NEGOTIATE_SIGN)
    {
        FIXME("Check SIGN\n");
    }

    /* Compute the HMAC-MD5 hash of ConcatenationOf(seq_num,data) using the client signing key */
    /*HMACMD5Init(&hmac, ctxMsg->ServerSigningKey, sizeof(ctxMsg->ServerSigningKey));
    HMACMD5Update(&hmac, (UCHAR*) &(MessageSeqNo), sizeof(MessageSeqNo));
    HMACMD5Update(&hmac, data_buffer->pvBuffer, data_buffer->cbBuffer);
    HMACMD5Final(&hmac, digest);*/
    NtlmFree(data);

    /* RC4-encrypt first 8 bytes of digest */
    /*rc4_crypt(ctxMsg->RecvSealKey, digest, checksum, 8);*/

    /* Concatenate version, ciphertext and sequence number to build signature */
    /*memcpy(expected_signature, (void*) &version, 4);
    memcpy(&expected_signature[4], (void*) checksum, 4);
    memcpy(&expected_signature[12], (void*) &(MessageSeqNo), 4);
    ctxMsg->RecvSequenceNum++;

    if (memcmp(signature->pvBuffer, expected_signature, sizeof(expected_signature)) != 0)
    {
        / * signature verification failed! * /
        ERR("signature verification failed, something nasty is going on!\n");
        ret = SEC_E_MESSAGE_ALTERED;
    }*/

exit:
    NtlmDereferenceContext(phContext->dwLower);
    return ret;
}
