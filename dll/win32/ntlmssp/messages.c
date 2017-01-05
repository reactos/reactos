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
    UCHAR digest[16], checksum[8], *signature;
    UINT index, length, version = 1;
    PNTLMSSP_CONTEXT context;
    HMAC_MD5_CTX hmac;
    void* data;

    ERR("EncryptMessage(%p %d %p %d)\n", phContext, fQOP, pMessage, MessageSeqNo);

    if(fQOP)
        FIXME("Ignoring fQOP\n");

    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    /* get context, need to free it later! */
    context = NtlmReferenceContext(phContext->dwLower);

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
    HMACMD5Init(&hmac, context->ClientSigningKey, sizeof(context->ServerSigningKey));
    HMACMD5Update(&hmac, (void*) &(MessageSeqNo), sizeof(MessageSeqNo));
    HMACMD5Update(&hmac, data, length);
    HMACMD5Final(&hmac, digest);

    /* Encrypt message using with RC4, result overwrites original buffer */
    rc4_crypt(context->SendSealKey, data_buffer->pvBuffer, data, length);
    NtlmFree(data);

    /* RC4-encrypt first 8 bytes of digest */
    rc4_crypt(context->SendSealKey, checksum, digest, 8);
    signature = (UCHAR*) signature_buffer->pvBuffer;

    /* Concatenate version, ciphertext and sequence number to build signature */
    memcpy(signature, (void*) &version, sizeof(version));
    memcpy(&signature[4], (void*) checksum, sizeof(checksum));
    memcpy(&signature[12], (void*) &(MessageSeqNo), sizeof(MessageSeqNo));
    context->SentSequenceNum++;

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
    PNTLMSSP_CONTEXT context;
    ULONG index, length;
    HMAC_MD5_CTX hmac;
    UINT version = 1;
    UCHAR digest[16], expected_signature[16], checksum[8];
    PSecBuffer data_buffer = NULL, signature = NULL;
    PVOID data;

    TRACE("(%p %p %d %p)\n", phContext, pMessage, MessageSeqNo, pfQOP);

    /* sanity checks */
    if(!phContext)
        return SEC_E_INVALID_HANDLE;

    if(!pMessage || !pMessage->pBuffers || pMessage->cBuffers < 2)
        return SEC_E_INVALID_TOKEN;

    /* get context */
    context = NtlmReferenceContext(phContext->dwLower);

    /* extract data and signature buffers */
    for (index = 0; index < (int) pMessage->cBuffers; index++)
    {
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

    
    if(signature->cbBuffer < 16)
    {
        ERR("Signature buffer too small!\n");
        ret = SEC_E_BUFFER_TOO_SMALL;
        goto exit;
    }
    
    /* Copy original data buffer */
    length = data_buffer->cbBuffer;
    data = NtlmAllocate(length);
    memcpy(data, data_buffer->pvBuffer, length);

    /* Decrypt message using with RC4 */
    rc4_crypt(context->RecvSealKey, data_buffer->pvBuffer, data, length);

    /* Compute the HMAC-MD5 hash of ConcatenationOf(seq_num,data) using the client signing key */
    HMACMD5Init(&hmac, context->ServerSigningKey, sizeof(context->ServerSigningKey));
    HMACMD5Update(&hmac, (UCHAR*) &(MessageSeqNo), sizeof(MessageSeqNo));
    HMACMD5Update(&hmac, data_buffer->pvBuffer, data_buffer->cbBuffer);
    HMACMD5Final(&hmac, digest);
    NtlmFree(data);

    /* RC4-encrypt first 8 bytes of digest */
    rc4_crypt(context->RecvSealKey, digest, checksum, 8);

    /* Concatenate version, ciphertext and sequence number to build signature */
    memcpy(expected_signature, (void*) &version, 4);
    memcpy(&expected_signature[4], (void*) checksum, 4);
    memcpy(&expected_signature[12], (void*) &(MessageSeqNo), 4);
    context->RecvSequenceNum++;

    if (memcmp(signature->pvBuffer, expected_signature, sizeof(expected_signature)) != 0)
    {
        /* signature verification failed! */
        ERR("signature verification failed, something nasty is going on!\n");
        ret = SEC_E_MESSAGE_ALTERED;
    }

exit:
    NtlmDereferenceContext(phContext->dwLower);
    return ret;
}
