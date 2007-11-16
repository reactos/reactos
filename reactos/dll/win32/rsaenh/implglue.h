/*
 * dlls/rsaenh/implglue.h
 * Glueing the RSAENH specific code to the crypto library
 *
 * Copyright (c) 2004 Michael Jung
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

#ifndef __WINE_IMPLGLUE_H
#define __WINE_IMPLGLUE_H

#include "tomcrypt.h"

/* Next typedef copied from dlls/advapi32/crypt_md4.c */
typedef struct tagMD4_CTX {
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

/* Next typedef copied from dlls/advapi32/crypt_md5.c */
typedef struct tagMD5_CTX
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

/* Next typedef copied form dlls/advapi32/crypt_sha.c */
typedef struct tagSHA_CTX
{
    ULONG Unknown[6];
    ULONG State[5];
    ULONG Count[2];
    UCHAR Buffer[64];
} SHA_CTX, *PSHA_CTX;

typedef union tagHASH_CONTEXT {
    md2_state md2;
    MD4_CTX md4;
    MD5_CTX md5;
    SHA_CTX sha;
} HASH_CONTEXT;

typedef union tagKEY_CONTEXT {
    rc2_key rc2;
    des_key des;
    des3_key des3;
    prng_state rc4;
    rsa_key rsa;
} KEY_CONTEXT;

BOOL init_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext);
BOOL update_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext, CONST BYTE *pbData, 
                      DWORD dwDataLen);
BOOL finalize_hash_impl(ALG_ID aiAlgid, HASH_CONTEXT *pHashContext, BYTE *pbHashValue);
BOOL duplicate_hash_impl(ALG_ID aiAlgid, CONST HASH_CONTEXT *pSrcHashContext, 
                         HASH_CONTEXT *pDestHashContext);

BOOL new_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen);
BOOL free_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext);
BOOL setup_key_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                    DWORD dwEffectiveKeyLen, DWORD dwSaltLen, BYTE *abKeyValue);
BOOL duplicate_key_impl(ALG_ID aiAlgid, CONST KEY_CONTEXT *pSrcKeyContext, 
                        KEY_CONTEXT *pDestKeyContext);

/* dwKeySpec is optional for symmetric key algorithms */
BOOL encrypt_block_impl(ALG_ID aiAlgid, DWORD dwKeySpec, KEY_CONTEXT *pKeyContext, CONST BYTE *pbIn, BYTE *pbOut, 
                        DWORD enc);
BOOL encrypt_stream_impl(ALG_ID aiAlgid, KEY_CONTEXT *pKeyContext, BYTE *pbInOut, DWORD dwLen);

BOOL export_public_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                            DWORD *pdwPubExp);
BOOL import_public_key_impl(CONST BYTE *pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen, 
                            DWORD dwPubExp);
BOOL export_private_key_impl(BYTE *pbDest, const KEY_CONTEXT *pKeyContext, DWORD dwKeyLen,
                             DWORD *pdwPubExp);
BOOL import_private_key_impl(CONST BYTE* pbSrc, KEY_CONTEXT *pKeyContext, DWORD dwKeyLen, 
                             DWORD dwPubExp);

BOOL gen_rand_impl(BYTE *pbBuffer, DWORD dwLen);

#endif /* __WINE_IMPLGLUE_H */
