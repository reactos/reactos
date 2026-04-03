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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"

#include "implglue.h"

/* Function prototype copied from dlls/advapi32/crypt.c */
BOOL WINAPI SystemFunction036(PVOID pbBuffer, ULONG dwLen);

BOOL init_hash_impl(ALG_ID aiAlgid, BCRYPT_HASH_HANDLE *hash_handle)
{
    switch (aiAlgid)
    {
    case CALG_MD2:
        return !BCryptCreateHash(BCRYPT_MD2_ALG_HANDLE, hash_handle, NULL, 0, NULL, 0, 0);

    case CALG_MD4:
        return !BCryptCreateHash(BCRYPT_MD4_ALG_HANDLE, hash_handle, NULL, 0, NULL, 0, 0);

    case CALG_MD5:
        return !BCryptCreateHash(BCRYPT_MD5_ALG_HANDLE, hash_handle, NULL, 0, NULL, 0, 0);

    case CALG_SHA:
        return !BCryptCreateHash(BCRYPT_SHA1_ALG_HANDLE, hash_handle, NULL, 0, NULL, 0, 0);

    case CALG_SHA_256:
        return !BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE, hash_handle, NULL, 0, NULL, 0, 0);

    case CALG_SHA_384:
        return !BCryptCreateHash(BCRYPT_SHA384_ALG_HANDLE, hash_handle, NULL, 0, NULL, 0, 0);

    case CALG_SHA_512:
        return !BCryptCreateHash(BCRYPT_SHA512_ALG_HANDLE, hash_handle, NULL, 0, NULL, 0, 0);

    default:
        return TRUE;
    }
}

BOOL update_hash_impl(BCRYPT_HASH_HANDLE hash_handle, const BYTE *pbData, DWORD dwDataLen)
{
    BCryptHashData(hash_handle, (UCHAR*)pbData, dwDataLen, 0);
    return TRUE;
}

BOOL finalize_hash_impl(BCRYPT_HASH_HANDLE hash_handle, BYTE *hash_value, DWORD hash_size)
{
    BCryptFinishHash(hash_handle, hash_value, hash_size, 0);
    BCryptDestroyHash(hash_handle);
    return TRUE;
}

BOOL duplicate_hash_impl(BCRYPT_HASH_HANDLE src_hash_handle, BCRYPT_HASH_HANDLE *dest_hash_handle)
{
    return !BCryptDuplicateHash(src_hash_handle, dest_hash_handle, NULL, 0, 0);
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

BOOL duplicate_key_impl(ALG_ID aiAlgid, const KEY_CONTEXT *pSrcKeyContext,
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

BOOL encrypt_block_impl(ALG_ID aiAlgid, DWORD dwKeySpec, KEY_CONTEXT *pKeyContext, const BYTE *in,
                        BYTE *out, DWORD enc)
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
        case CALG_SSL3_SHAMD5:
            outlen = inlen = (mp_count_bits(&pKeyContext->rsa.N)+7)/8;
            if (enc) {
                if (rsa_exptmod(in, inlen, out, &outlen, dwKeySpec, &pKeyContext->rsa) != CRYPT_OK) {
                    SetLastError(NTE_FAIL);
                    return FALSE;
                }
                reverse_bytes(out, outlen);
            } else {
                in_reversed = malloc(inlen);
                if (!in_reversed) {
                    SetLastError(NTE_NO_MEMORY);
                    return FALSE;
                }
                memcpy(in_reversed, in, inlen);
                reverse_bytes(in_reversed, inlen);
                if (rsa_exptmod(in_reversed, inlen, out, &outlen, dwKeySpec, &pKeyContext->rsa) != CRYPT_OK) {
                    free(in_reversed);
                    SetLastError(NTE_FAIL);
                    return FALSE;
                }
                free(in_reversed);
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
    reverse_bytes(pbDest, mp_unsigned_bin_size(&pKeyContext->rsa.N));
    if (mp_unsigned_bin_size(&pKeyContext->rsa.N) < dwKeyLen)
        memset(pbDest + mp_unsigned_bin_size(&pKeyContext->rsa.N), 0,
               dwKeyLen - mp_unsigned_bin_size(&pKeyContext->rsa.N));
    *pdwPubExp = (DWORD)mp_get_int(&pKeyContext->rsa.e);
    return TRUE;
}

BOOL import_public_key_impl(const BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
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

    pbTemp = malloc(dwKeyLen);
    if (!pbTemp) return FALSE;
    memcpy(pbTemp, pbSrc, dwKeyLen);
    
    pKeyContext->rsa.type = PK_PUBLIC;
    reverse_bytes(pbTemp, dwKeyLen);
    mp_read_unsigned_bin(&pKeyContext->rsa.N, pbTemp, dwKeyLen);
    free(pbTemp);
    mp_set_int(&pKeyContext->rsa.e, dwPubExp);

    return TRUE;    
}

BOOL export_private_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                             DWORD *pdwPubExp)
{
    mp_to_unsigned_bin(&pKeyContext->rsa.N, pbDest);
    reverse_bytes(pbDest, mp_unsigned_bin_size(&pKeyContext->rsa.N));
    if (mp_unsigned_bin_size(&pKeyContext->rsa.N) < dwKeyLen)
        memset(pbDest + mp_unsigned_bin_size(&pKeyContext->rsa.N), 0,
               dwKeyLen - mp_unsigned_bin_size(&pKeyContext->rsa.N));
    pbDest += dwKeyLen;
    mp_to_unsigned_bin(&pKeyContext->rsa.p, pbDest);
    reverse_bytes(pbDest, mp_unsigned_bin_size(&pKeyContext->rsa.p));
    if (mp_unsigned_bin_size(&pKeyContext->rsa.p) < (dwKeyLen+1)>>1)
        memset(pbDest + mp_unsigned_bin_size(&pKeyContext->rsa.p), 0,
               ((dwKeyLen+1)>>1) - mp_unsigned_bin_size(&pKeyContext->rsa.p));
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.q, pbDest);
    reverse_bytes(pbDest, mp_unsigned_bin_size(&pKeyContext->rsa.q));
    if (mp_unsigned_bin_size(&pKeyContext->rsa.q) < (dwKeyLen+1)>>1)
        memset(pbDest + mp_unsigned_bin_size(&pKeyContext->rsa.q), 0,
               ((dwKeyLen+1)>>1) - mp_unsigned_bin_size(&pKeyContext->rsa.q));
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.dP, pbDest);
    reverse_bytes(pbDest, mp_unsigned_bin_size(&pKeyContext->rsa.dP));
    if (mp_unsigned_bin_size(&pKeyContext->rsa.dP) < (dwKeyLen+1)>>1)
        memset(pbDest + mp_unsigned_bin_size(&pKeyContext->rsa.dP), 0,
               ((dwKeyLen+1)>>1) - mp_unsigned_bin_size(&pKeyContext->rsa.dP));
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.dQ, pbDest);
    reverse_bytes(pbDest, mp_unsigned_bin_size(&pKeyContext->rsa.dQ));
    if (mp_unsigned_bin_size(&pKeyContext->rsa.dQ) < (dwKeyLen+1)>>1)
        memset(pbDest + mp_unsigned_bin_size(&pKeyContext->rsa.dQ), 0,
               ((dwKeyLen+1)>>1) - mp_unsigned_bin_size(&pKeyContext->rsa.dQ));
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.qP, pbDest);
    reverse_bytes(pbDest, mp_unsigned_bin_size(&pKeyContext->rsa.qP));
    if (mp_unsigned_bin_size(&pKeyContext->rsa.qP) < (dwKeyLen+1)>>1)
        memset(pbDest + mp_unsigned_bin_size(&pKeyContext->rsa.qP), 0,
               ((dwKeyLen+1)>>1) - mp_unsigned_bin_size(&pKeyContext->rsa.qP));
    pbDest += (dwKeyLen+1)>>1;
    mp_to_unsigned_bin(&pKeyContext->rsa.d, pbDest);
    reverse_bytes(pbDest, mp_unsigned_bin_size(&pKeyContext->rsa.d));
    if (mp_unsigned_bin_size(&pKeyContext->rsa.d) < dwKeyLen)
        memset(pbDest + mp_unsigned_bin_size(&pKeyContext->rsa.d), 0,
               dwKeyLen - mp_unsigned_bin_size(&pKeyContext->rsa.d));
    *pdwPubExp = (DWORD)mp_get_int(&pKeyContext->rsa.e);

    return TRUE;
}

BOOL import_private_key_impl(const BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                             DWORD dwDataLen, DWORD dwPubExp)
{
    BYTE *pbTemp, *pbBigNum;

    if (mp_init_multi(&pKeyContext->rsa.e, &pKeyContext->rsa.d, &pKeyContext->rsa.N, 
                      &pKeyContext->rsa.dQ,&pKeyContext->rsa.dP,&pKeyContext->rsa.qP, 
                      &pKeyContext->rsa.p, &pKeyContext->rsa.q, NULL) != MP_OKAY)
    {
        SetLastError(NTE_FAIL);
        return FALSE;
    }

    pbTemp = malloc(2*dwKeyLen+5*((dwKeyLen+1)>>1));
    if (!pbTemp) return FALSE;
    memcpy(pbTemp, pbSrc, min(dwDataLen, 2*dwKeyLen+5*((dwKeyLen+1)>>1)));
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
    /* The size of the private exponent d is inferred from the remaining
     * data length.
     */
    dwKeyLen = min(dwKeyLen, dwDataLen - (pbBigNum - pbTemp));
    reverse_bytes(pbBigNum, dwKeyLen);
    mp_read_unsigned_bin(&pKeyContext->rsa.d, pbBigNum, dwKeyLen);
    mp_set_int(&pKeyContext->rsa.e, dwPubExp);
    
    free(pbTemp);
    return TRUE;
}
