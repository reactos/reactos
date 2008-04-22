/*
 * dlls/rsaenh/implglue.c
 * Glueing the RSAENH specific code to the crypto library
 *
 * Copyright (c) 2004, 2005 Michael Jung
 * Copyright (c) 2007 Vijay Kiran Kamuju
 *
 * based on code by Mike McCormack and David Hammerton
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
 */
#include "config.h"

#include "wine/port.h"
#include "wine/library.h"

#include "windef.h"
#include "wincrypt.h"

#include "implglue.h"

#include <stdio.h>

/* Function prototypes copied from dlls/advapi32/crypt_md4.c */
VOID WINAPI MD4Init( MD4_CTX *ctx );
VOID WINAPI MD4Update( MD4_CTX *ctx, const unsigned char *buf, unsigned int len );
VOID WINAPI MD4Final( MD4_CTX *ctx );
/* Function prototypes copied from dlls/advapi32/crypt_md5.c */
VOID WINAPI MD5Init( MD5_CTX *ctx );
VOID WINAPI MD5Update( MD5_CTX *ctx, const unsigned char *buf, unsigned int len );
VOID WINAPI MD5Final( MD5_CTX *ctx );
/* Function prototypes copied from dlls/advapi32/crypt_sha.c */
VOID WINAPI A_SHAInit(PSHA_CTX Context);
VOID WINAPI A_SHAUpdate(PSHA_CTX Context, const unsigned char *Buffer, UINT BufferSize);
VOID WINAPI A_SHAFinal(PSHA_CTX Context, PULONG Result);
/* Function prototype copied from dlls/advapi32/crypt.c */
BOOL WINAPI SystemFunction036(PVOID pbBuffer, ULONG dwLen);
        
BOOL init_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext) 
{
    switch (aiAlgid) 
    {
        case CALG_MD2:
            md2_init(&pHashContext->md2);
            break;
        
        case CALG_MD4:
            MD4Init(&pHashContext->md4);
            break;
        
        case CALG_MD5:
            MD5Init(&pHashContext->md5);
            break;
        
        case CALG_SHA:
            A_SHAInit(&pHashContext->sha);
            break;
    }

    return TRUE;
}

BOOL update_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext, CONST BYTE *pbData, 
                      DWORD dwDataLen) 
{
    switch (aiAlgid)
    {
        case CALG_MD2:
            md2_process(&pHashContext->md2, pbData, dwDataLen);
            break;
        
        case CALG_MD4:
            MD4Update(&pHashContext->md4, pbData, dwDataLen);
            break;
    
        case CALG_MD5:
            MD5Update(&pHashContext->md5, pbData, dwDataLen);
            break;
        
        case CALG_SHA:
            A_SHAUpdate(&pHashContext->sha, pbData, dwDataLen);
            break;
        
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL finalize_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext, BYTE *pbHashValue) 
{
    switch (aiAlgid)
    {
        case CALG_MD2:
            md2_done(&pHashContext->md2, pbHashValue);
            break;
        
        case CALG_MD4:
            MD4Final(&pHashContext->md4);
            memcpy(pbHashValue, pHashContext->md4.digest, 16);
            break;
        
        case CALG_MD5:
            MD5Final(&pHashContext->md5);
            memcpy(pbHashValue, pHashContext->md5.digest, 16);
            break;
        
        case CALG_SHA:
            A_SHAFinal(&pHashContext->sha, (PULONG)pbHashValue);
            break;
        
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL duplicate_hash_impl(ALG_ID aiAlgid, CONST HASH_CONTEXT *pSrcHashContext, 
                         HASH_CONTEXT *pDestHashContext) 
{
    *pDestHashContext = *pSrcHashContext;

    return TRUE;
}

BOOL new_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen) 
{
    switch (aiAlgid)
    {
        case CALG_RSA_KEYX:
        case CALG_RSA_SIGN:
            if (rsa_make_key((int)dwKeyLen, 65537, &pKeyContext->rsa) != CRYPT_OK) {
                SetLastError(NTE_FAIL);
                return FALSE;
            }
            return TRUE;
    }

    return TRUE;
}

BOOL free_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext)
{
    switch (aiAlgid)
    {
        case CALG_RSA_KEYX:
        case CALG_RSA_SIGN:
            rsa_free(&pKeyContext->rsa);
    }

    return TRUE;
}

BOOL setup_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                    DWORD dwEffectiveKeyLen, DWORD dwSaltLen, BYTE *abKeyValue)
{
    switch (aiAlgid) 
    {
        case CALG_RC4:
            rc4_start(&pKeyContext->rc4);
            rc4_add_entropy(abKeyValue, dwKeyLen + dwSaltLen, &pKeyContext->rc4);
            rc4_ready(&pKeyContext->rc4);
            break;
        
        case CALG_RC2:
            rc2_setup(abKeyValue, dwKeyLen + dwSaltLen, dwEffectiveKeyLen ?
                      dwEffectiveKeyLen : dwKeyLen << 3, 0, &pKeyContext->rc2);
            break;
        
        case CALG_3DES:
            des3_setup(abKeyValue, 24, 0, &pKeyContext->des3);
            break;

        case CALG_3DES_112:
            memcpy(abKeyValue+16, abKeyValue, 8);
            des3_setup(abKeyValue, 24, 0, &pKeyContext->des3);
            break;
        
        case CALG_DES:
            des_setup(abKeyValue, 8, 0, &pKeyContext->des);
            break;

        case CALG_AES:
        case CALG_AES_128:
            aes_setup(abKeyValue, 16, 0, &pKeyContext->aes);
            break;

        case CALG_AES_192:
            aes_setup(abKeyValue, 24, 0, &pKeyContext->aes);
            break;

        case CALG_AES_256:
            aes_setup(abKeyValue, 32, 0, &pKeyContext->aes);
            break;
    }

    return TRUE;
}

BOOL duplicate_key_impl(ALG_ID aiAlgid, CONST KEY_CONTEXT *pSrcKeyContext,
                        KEY_CONTEXT *pDestKeyContext) 
{
    switch (aiAlgid) 
    {
        case CALG_RC4:
        case CALG_RC2:
        case CALG_3DES:
        case CALG_3DES_112:
        case CALG_DES:
        case CALG_AES:
        case CALG_AES_128:
        case CALG_AES_192:
        case CALG_AES_256:
            *pDestKeyContext = *pSrcKeyContext;
            break;
        case CALG_RSA_KEYX:
        case CALG_RSA_SIGN:
            pDestKeyContext->rsa.type = pSrcKeyContext->rsa.type;
            mp_init_copy(&pDestKeyContext->rsa.e, &pSrcKeyContext->rsa.e);
            mp_init_copy(&pDestKeyContext->rsa.d, &pSrcKeyContext->rsa.d);
            mp_init_copy(&pDestKeyContext->rsa.N, &pSrcKeyContext->rsa.N);
            mp_init_copy(&pDestKeyContext->rsa.p, &pSrcKeyContext->rsa.p);
            mp_init_copy(&pDestKeyContext->rsa.q, &pSrcKeyContext->rsa.q);
            mp_init_copy(&pDestKeyContext->rsa.qP, &pSrcKeyContext->rsa.qP);
            mp_init_copy(&pDestKeyContext->rsa.dP, &pSrcKeyContext->rsa.dP);
            mp_init_copy(&pDestKeyContext->rsa.dQ, &pSrcKeyContext->rsa.dQ);
            break;
        
        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

static inline void reverse_bytes(BYTE *pbData, DWORD dwLen) {
    BYTE swap;
    DWORD i;

    for (i=0; i<dwLen/2; i++) {
        swap = pbData[i];
        pbData[i] = pbData[dwLen-i-1];
        pbData[dwLen-i-1] = swap;
    }
}

BOOL encrypt_block_impl(ALG_ID aiAlgid, DWORD dwKeySpec, KEY_CONTEXT *pKeyContext, CONST BYTE *in, BYTE *out, 
                        DWORD enc) 
{
    unsigned long inlen, outlen;
    BYTE *in_reversed = NULL;
        
    switch (aiAlgid) {
        case CALG_RC2:
            if (enc) {
                rc2_ecb_encrypt(in, out, &pKeyContext->rc2);
            } else {
                rc2_ecb_decrypt(in, out, &pKeyContext->rc2);
            }
            break;

        case CALG_3DES:
        case CALG_3DES_112:
            if (enc) {
                des3_ecb_encrypt(in, out, &pKeyContext->des3);
            } else {
                des3_ecb_decrypt(in, out, &pKeyContext->des3);
            }
            break;

        case CALG_DES:
            if (enc) {
                des_ecb_encrypt(in, out, &pKeyContext->des);
            } else {
                des_ecb_decrypt(in, out, &pKeyContext->des);
            }
            break;

        case CALG_AES:
        case CALG_AES_128:
        case CALG_AES_192:
        case CALG_AES_256:
            if (enc) {
                aes_ecb_encrypt(in, out, &pKeyContext->aes);
            } else {
                aes_ecb_decrypt(in, out, &pKeyContext->aes);
            }
            break;

        case CALG_RSA_KEYX:
        case CALG_RSA_SIGN:
            outlen = inlen = (mp_count_bits(&pKeyContext->rsa.N)+7)/8;
            if (enc) {
                if (rsa_exptmod(in, inlen, out, &outlen, dwKeySpec, &pKeyContext->rsa) != CRYPT_OK) {
                    SetLastError(NTE_FAIL);
                    return FALSE;
                }
                reverse_bytes(out, outlen);
            } else {
                in_reversed = HeapAlloc(GetProcessHeap(), 0, inlen);
                if (!in_reversed) {
                    SetLastError(NTE_NO_MEMORY);
                    return FALSE;
                }
                memcpy(in_reversed, in, inlen);
                reverse_bytes(in_reversed, inlen);
                if (rsa_exptmod(in_reversed, inlen, out, &outlen, dwKeySpec, &pKeyContext->rsa) != CRYPT_OK) {
                    HeapFree(GetProcessHeap(), 0, in_reversed);
                    SetLastError(NTE_FAIL);
                    return FALSE;
                }
                HeapFree(GetProcessHeap(), 0, in_reversed);
            }
            break;

        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL encrypt_stream_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, BYTE *stream, DWORD dwLen)
{
    switch (aiAlgid) {
        case CALG_RC4:
            rc4_read(stream, dwLen, &pKeyContext->rc4);
            break;

        default:
            SetLastError(NTE_BAD_ALGID);
            return FALSE;
    }

    return TRUE;
}

BOOL gen_rand_impl(BYTE *pbBuffer, DWORD dwLen)
{
    return SystemFunction036(pbBuffer, dwLen);
}

BOOL export_public_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,DWORD *pdwPubExp)
{
    mp_to_unsigned_bin(&pKeyContext->rsa.N, pbDest);
    reverse_bytes(pbDest, dwKeyLen);
    *pdwPubExp = (DWORD)mp_get_int(&pKeyContext->rsa.e);
    return TRUE;
}

BOOL import_public_key_impl(CONST BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen, 
                            DWORD dwPubExp)
{
    BYTE *pbTemp;

    if (mp_init_multi(&pKeyContext->rsa.e, &pKeyContext->rsa.d, &pKeyContext->rsa.N, 
                      &pKeyContext->rsa.dQ,&pKeyContext->rsa.dP,&pKeyContext->rsa.qP, 
                      &pKeyContext->rsa.p, &pKeyContext->rsa.q, NULL) != MP_OKAY)
    {
        SetLastError(NTE_FAIL);
        return FALSE;
    }

    pbTemp = HeapAlloc(GetProcessHeap(), 0, dwKeyLen);
    if (!pbTemp) return FALSE;
    memcpy(pbTemp, pbSrc, dwKeyLen);
    
    pKeyContext->rsa.type = PK_PUBLIC;
    reverse_bytes(pbTemp, dwKeyLen);
    mp_read_unsigned_bin(&pKeyContext->rsa.N, pbTemp, dwKeyLen);
    HeapFree(GetProcessHeap(), 0, pbTemp);
    mp_set_int(&pKeyContext->rsa.e, dwPubExp);

    return TRUE;    
}

BOOL export_private_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                             DWORD *pdwPubExp)
{
    mp_to_unsigned_bin(&pKeyContext->rsa.N, pbDest);
    reverse_bytes(pbDest, dwKeyLen);
    pbDest += dwKeyLen;
    mp_to_unsigned_bin(&pKeyContext->rsa.p, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.q, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.dP, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.dQ, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.qP, pbDest);
    reverse_bytes(pbDest, (dwKeyLen+1)>>1);
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.d, pbDest);
    reverse_bytes(pbDest, dwKeyLen);
    *pdwPubExp = (DWORD)mp_get_int(&pKeyContext->rsa.e);

    return TRUE;
}

BOOL import_private_key_impl(CONST BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen, 
                             DWORD dwPubExp)
{
    BYTE *pbTemp, *pbBigNum;

    if (mp_init_multi(&pKeyContext->rsa.e, &pKeyContext->rsa.d, &pKeyContext->rsa.N, 
                      &pKeyContext->rsa.dQ,&pKeyContext->rsa.dP,&pKeyContext->rsa.qP, 
                      &pKeyContext->rsa.p, &pKeyContext->rsa.q, NULL) != MP_OKAY)
    {
        SetLastError(NTE_FAIL);
        return FALSE;
    }

    pbTemp = HeapAlloc(GetProcessHeap(), 0, 2*dwKeyLen+5*((dwKeyLen+1)>>1));
    if (!pbTemp) return FALSE;
    memcpy(pbTemp, pbSrc, 2*dwKeyLen+5*((dwKeyLen+1)>>1));
    pbBigNum = pbTemp;

    pKeyContext->rsa.type = PK_PRIVATE;
    reverse_bytes(pbBigNum, dwKeyLen);
    mp_read_unsigned_bin(&pKeyContext->rsa.N, pbBigNum, dwKeyLen);
    pbBigNum += dwKeyLen;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    mp_read_unsigned_bin(&pKeyContext->rsa.p, pbBigNum, (dwKeyLen+1)>>1);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    mp_read_unsigned_bin(&pKeyContext->rsa.q, pbBigNum, (dwKeyLen+1)>>1);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    mp_read_unsigned_bin(&pKeyContext->rsa.dP, pbBigNum, (dwKeyLen+1)>>1);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    mp_read_unsigned_bin(&pKeyContext->rsa.dQ, pbBigNum, (dwKeyLen+1)>>1);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, (dwKeyLen+1)>>1);
    mp_read_unsigned_bin(&pKeyContext->rsa.qP, pbBigNum, (dwKeyLen+1)>>1);
    pbBigNum += (dwKeyLen+1)>>1;
    reverse_bytes(pbBigNum, dwKeyLen);
    mp_read_unsigned_bin(&pKeyContext->rsa.d, pbBigNum, dwKeyLen);
    mp_set_int(&pKeyContext->rsa.e, dwPubExp);
    
    HeapFree(GetProcessHeap(), 0, pbTemp);
    return TRUE;
}
