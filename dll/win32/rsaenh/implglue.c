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
#include "ntsecapi.h"
#include "wincrypt.h"

#include "implglue.h"

SYMCRYPT_ENVIRONMENT_DEFS( WindowsUsermodeWin8_1nLater );

void * SYMCRYPT_CALL SymCryptCallbackAlloc( SIZE_T size )
{
    return _aligned_malloc( size, SYMCRYPT_ASYM_ALIGN_VALUE );
}

void SYMCRYPT_CALL SymCryptCallbackFree( void *ptr )
{
    _aligned_free( ptr );
}

void SYMCRYPT_CALL SymCryptRsaSelftest( void )
{
}

SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptRsaSignVerifyPct( const SYMCRYPT_RSAKEY *key )
{
    return SYMCRYPT_NO_ERROR;
}

SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptCallbackRandom( BYTE *buf, SIZE_T size )
{
    return RtlGenRandom( buf, size ) ? SYMCRYPT_NO_ERROR : SYMCRYPT_EXTERNAL_FAILURE;
}

UINT32 g_SymCryptFipsSelftestsPerformed;

BOOL init_hash_impl( ALG_ID algid, struct hash *hash )
{
    const SYMCRYPT_HASH *algorithms[] =
    {
        [ALG_SID_MD2] = SymCryptMd2Algorithm,
        [ALG_SID_MD4] = SymCryptMd4Algorithm,
        [ALG_SID_MD5] = SymCryptMd5Algorithm,
        [ALG_SID_SHA] = SymCryptSha1Algorithm,
        [ALG_SID_SHA_256] = SymCryptSha256Algorithm,
        [ALG_SID_SHA_384] = SymCryptSha384Algorithm,
        [ALG_SID_SHA_512] = SymCryptSha512Algorithm,
    };

    memset( hash, 0, sizeof(*hash) );
    if (GET_ALG_CLASS(algid) != ALG_CLASS_HASH) return TRUE;
    if (GET_ALG_SID(algid) >= ARRAY_SIZE(algorithms)) return TRUE;
    if (!(hash->desc = algorithms[GET_ALG_SID(algid)])) return TRUE;
    SymCryptHashInit( hash->desc, &hash->state );
    return TRUE;
}

static SYMCRYPT_RSAKEY *alloc_rsa_key( DWORD bitlen, BOOL private )
{
    SYMCRYPT_RSA_PARAMS params;

    params.version = 1;
    params.nBitsOfModulus = bitlen;
    params.nPrimes = private ? 2 : 0;
    params.nPubExp = 1;
    return SymCryptRsakeyAllocate( &params, 0 );
}

BOOL new_key_impl( ALG_ID algid, KEY_CONTEXT *ctx, DWORD keylen )
{
    switch (algid)
    {
    case CALG_RSA_KEYX:
    case CALG_RSA_SIGN:
    {
        UINT64 pubexp = 65537;

        ctx->rsa.flags = SYMCRYPT_FLAG_KEY_NO_FIPS;
        if (algid == CALG_RSA_KEYX) ctx->rsa.flags |= SYMCRYPT_FLAG_RSAKEY_ENCRYPT;
        else ctx->rsa.flags |= SYMCRYPT_FLAG_RSAKEY_SIGN;

        if (!(ctx->rsa.key = alloc_rsa_key( keylen * 8, TRUE )))
        {
            SetLastError( NTE_FAIL );
            return FALSE;
        }
        if (SymCryptRsakeyGenerate( ctx->rsa.key, &pubexp, 1, ctx->rsa.flags ))
        {
            SymCryptRsakeyFree( ctx->rsa.key );
            ctx->rsa.key = NULL;
            SetLastError( NTE_FAIL );
            return FALSE;
        }
        break;
    }
    default:
        SetLastError( NTE_BAD_ALGID );
        return FALSE;
    }
    return TRUE;
}

BOOL free_key_impl( ALG_ID algid, KEY_CONTEXT *ctx )
{
    switch (algid)
    {
    case CALG_RSA_KEYX:
    case CALG_RSA_SIGN:
        if (ctx->rsa.key)
        {
            SymCryptRsakeyFree( ctx->rsa.key );
            ctx->rsa.key = NULL;
        }
        break;
    default:
        break;
    }
    return TRUE;
}

BOOL setup_key_impl( ALG_ID algid, KEY_CONTEXT *ctx, DWORD keylen, DWORD effective_keylen, DWORD saltlen,
                     const BYTE *secret )
{
    switch (algid)
    {
    case CALG_RC4:
        SymCryptRc4Init( &ctx->rc4, secret, keylen + saltlen );
        break;
    case CALG_RC2:
        SymCryptRc2ExpandKeyEx( &ctx->rc2, secret, keylen + saltlen,
                                effective_keylen ? effective_keylen : keylen << 3 );
        break;
    case CALG_3DES:
        SymCrypt3DesExpandKey( &ctx->des3, secret, 24 );
        break;
    case CALG_3DES_112:
        SymCrypt3DesExpandKey( &ctx->des3, secret, 16 );
        break;
    case CALG_DES:
        SymCryptDesExpandKey( &ctx->des, secret, 8 );
        break;
    case CALG_AES:
    case CALG_AES_128:
        SymCryptAesExpandKey( &ctx->aes, secret, 16 );
        break;
    case CALG_AES_192:
        SymCryptAesExpandKey( &ctx->aes, secret, 24 );
        break;
    case CALG_AES_256:
        SymCryptAesExpandKey( &ctx->aes, secret, 32 );
        break;
    }

    return TRUE;
}

BOOL duplicate_key_impl( ALG_ID algid, const KEY_CONTEXT *src, KEY_CONTEXT *dst )
{
    switch (algid)
    {
    case CALG_RC4:
    case CALG_RC2:
    case CALG_3DES:
    case CALG_3DES_112:
    case CALG_DES:
        *dst = *src;
        break;
    case CALG_AES:
    case CALG_AES_128:
    case CALG_AES_192:
    case CALG_AES_256:
        /* SYMCRYPT_AES_EXPANDED_KEY contains pointers into its own RoundKey
         * array, so it cannot be copied by assignment. */
        SymCryptAesKeyCopy( &src->aes, &dst->aes );
        break;
    case CALG_RSA_KEYX:
    case CALG_RSA_SIGN:
    {
        BYTE *blob;
        DWORD pubexp, keylen = SymCryptRsakeySizeofModulus( src->rsa.key );
        BOOLEAN private = SymCryptRsakeyHasPrivateKey( src->rsa.key );

        if (!(dst->rsa.key = alloc_rsa_key( keylen * 8, private )) || !(blob = malloc( keylen * 2 )))
        {
            SetLastError( NTE_FAIL );
            return FALSE;
        }
        if (private)
        {
            if (!export_private_key_impl( src, blob, &pubexp ) ||
                !import_private_key_impl( algid, blob, keylen, pubexp, dst ))
            {
                free( blob );
                return FALSE;
            }
        }
        else
        {
            if (!export_public_key_impl( src, blob, &pubexp ) ||
                !import_public_key_impl( algid, blob, keylen, pubexp, dst ))
            {
                free( blob );
                return FALSE;
            }
        }
        dst->rsa.flags = src->rsa.flags;
        free( blob );
        break;
    }
    default:
        SetLastError( NTE_BAD_ALGID );
        return FALSE;
    }
    return TRUE;
}

static BOOL verify_input( const SYMCRYPT_RSAKEY *key, const BYTE *src, SIZE_T src_size, SYMCRYPT_NUMBER_FORMAT format,
                          BYTE *scratch, SIZE_T scratch_size )
{
    SYMCRYPT_INT *tmp;
    UINT32 tmp_size;

    if (src_size > SymCryptRsakeySizeofModulus( key )) return FALSE;
    if (src_size == SymCryptRsakeySizeofModulus( key ))
    {
        tmp_size = SymCryptSizeofIntFromDigits( key->nDigitsOfModulus );
        SYMCRYPT_ASSERT( scratch_size >= tmp_size );
        tmp = SymCryptIntCreate( scratch, tmp_size, key->nDigitsOfModulus );

        if (SymCryptIntSetValue( src, src_size, format, tmp )) return FALSE;
        if (!SymCryptIntIsLessThan( tmp, SymCryptIntFromModulus( key->pmModulus ))) return FALSE;
        return TRUE;
    }
    return FALSE;
}

static BOOL rsa_encrypt( const SYMCRYPT_RSAKEY *key, const BYTE *in, SYMCRYPT_NUMBER_FORMAT in_format,
                         BYTE *out, SYMCRYPT_NUMBER_FORMAT out_format )
{
    BYTE buf[SYMCRYPT_SIZEOF_INT_FROM_BITS(64) + SYMCRYPT_ASYM_ALIGN_VALUE];
    SYMCRYPT_MODELEMENT *result;
    SIZE_T scratch_size, mod_size = SymCryptSizeofModElementFromModulus( key->pmModulus );
    SIZE_T key_size = SymCryptRsakeySizeofModulus( key );
    SYMCRYPT_INT *exp;
    BYTE *scratch;

    scratch_size = SymCryptSizeofModElementFromModulus( key->pmModulus ) +
        SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( key->nDigitsOfModulus ),
                      SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( key->nDigitsOfModulus ) );

    if (!(scratch = malloc( scratch_size ))) return FALSE;
    if (!verify_input( key, in, key_size, in_format, scratch, scratch_size ))
    {
        free( scratch );
        return FALSE;
    }

    result = SymCryptModElementCreate( scratch, mod_size, key->pmModulus );

    SymCryptModElementSetValue( in, key_size, in_format, key->pmModulus, result, scratch + mod_size,
                                scratch_size - mod_size );

    exp = SymCryptIntCreate( SYMCRYPT_ASYM_ALIGN_UP(buf), sizeof(buf) - SYMCRYPT_ASYM_ALIGN_VALUE, 1 );
    SymCryptIntSetValueUint64( key->au64PubExp[0], exp );

    SymCryptModExp( key->pmModulus, result, exp, SymCryptIntBitsizeOfValue(exp), SYMCRYPT_FLAG_DATA_PUBLIC,
                    result, scratch + mod_size, scratch_size - mod_size );

    SymCryptModElementGetValue( key->pmModulus, result, out, key_size, out_format, scratch + mod_size,
                                scratch_size - mod_size );
    free( scratch );
    return TRUE;
}

BOOL encrypt_block_impl( ALG_ID algid, KEY_CONTEXT *ctx, const BYTE *in, BYTE *out )
{
    switch (algid)
    {
    case CALG_RC2:
        SymCryptRc2Encrypt( &ctx->rc2, in, out );
        break;
    case CALG_3DES:
    case CALG_3DES_112:
        SymCrypt3DesEncrypt( &ctx->des3, in, out );
        break;
    case CALG_DES:
        SymCryptDesEncrypt( &ctx->des, in, out );
        break;
    case CALG_AES:
    case CALG_AES_128:
    case CALG_AES_192:
    case CALG_AES_256:
        SymCryptAesEncrypt( &ctx->aes, in, out );
        break;
    case CALG_RSA_KEYX:
    case CALG_RSA_SIGN:
    case CALG_SSL3_SHAMD5:
        return rsa_encrypt( ctx->rsa.key, in, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, out, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST );
    default:
        SetLastError( NTE_BAD_ALGID );
        return FALSE;
    }
    return TRUE;
}

static BOOL rsa_decrypt( const SYMCRYPT_RSAKEY *key, const BYTE *in, SYMCRYPT_NUMBER_FORMAT in_format,
                         BYTE *out, SYMCRYPT_NUMBER_FORMAT out_format )
{
    SIZE_T scratch_size, offset = 0, int_size, tmp_size, key_size = SymCryptRsakeySizeofModulus( key );
    SYMCRYPT_INT *ciphertext, *plaintext, *tmp;
    SYMCRYPT_MODELEMENT *crt_elements[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];
    BYTE *scratch;
    UINT32 i, crt_element_total = 0, crt_elements_size[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];

    int_size = SymCryptSizeofIntFromDigits( key->nDigitsOfModulus );
    tmp_size = SymCryptSizeofIntFromDigits( key->nMaxDigitsOfPrimes );

    for (i = 0; i < key->nPrimes; i++)
    {
        crt_elements_size[i] = SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( key->nBitsOfPrimes[i] );
        crt_element_total += crt_elements_size[i];
    }

    scratch_size = 3 * int_size + tmp_size + crt_element_total +
        SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( key->nDigitsOfModulus ),
            SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( key->nDigitsOfModulus ),
                SYMCRYPT_MAX( SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( key->nDigitsOfModulus, key->nMaxDigitsOfPrimes ),
                    SYMCRYPT_SCRATCH_BYTES_FOR_CRT_SOLUTION( key->nMaxDigitsOfPrimes ) ) ) );

    if (!(scratch = malloc( scratch_size ))) return FALSE;

    ciphertext = SymCryptIntCreate( scratch, scratch_size, key->nDigitsOfModulus );
    offset += int_size;

    plaintext = SymCryptIntCreate( scratch + offset, scratch_size - offset, key->nDigitsOfModulus );
    offset += int_size;

    tmp = SymCryptIntCreate( scratch + offset, scratch_size - offset, key->nMaxDigitsOfPrimes );
    offset += tmp_size;

    for (i = 0; i < key->nPrimes; i++)
    {
        crt_elements[i] = SymCryptModElementCreate( scratch + offset, scratch_size - offset, key->pmPrimes[i] );
        offset += crt_elements_size[i];
    }

    SymCryptIntSetValue( in, key_size, in_format, ciphertext );

    for (i = 0; i < key->nPrimes; i++)
    {
        SymCryptIntDivMod( ciphertext, SymCryptDivisorFromModulus(key->pmPrimes[i]), NULL, tmp, scratch + offset,
                           scratch_size - offset );

        SymCryptIntToModElement( tmp, key->pmPrimes[i], crt_elements[i], scratch + offset, scratch_size - offset );

        SymCryptModExp( key->pmPrimes[i], crt_elements[i], key->piCrtPrivExps[i], key->nBitsOfPrimes[i], 0,
                        crt_elements[i], scratch + offset, scratch_size - offset );
    }

    SymCryptCrtSolve( key->nPrimes, (const SYMCRYPT_MODULUS **)key->pmPrimes,
                      (SYMCRYPT_MODELEMENT **)key->peCrtInverses, crt_elements, 0, plaintext, scratch + offset,
                      scratch_size - offset );

    SymCryptIntGetValue( plaintext, out, key_size, out_format );
    free( scratch );
    return TRUE;
}

BOOL decrypt_block_impl( ALG_ID algid, KEY_CONTEXT *ctx, const BYTE *in, BYTE *out )
{
    switch (algid)
    {
    case CALG_RC2:
        SymCryptRc2Decrypt( &ctx->rc2, in, out );
        break;
    case CALG_3DES:
    case CALG_3DES_112:
        SymCrypt3DesDecrypt( &ctx->des3, in, out );
        break;
    case CALG_DES:
        SymCryptDesDecrypt( &ctx->des, in, out );
        break;
    case CALG_AES:
    case CALG_AES_128:
    case CALG_AES_192:
    case CALG_AES_256:
        SymCryptAesDecrypt( &ctx->aes, in, out );
        break;
    case CALG_RSA_KEYX:
    case CALG_RSA_SIGN:
    case CALG_SSL3_SHAMD5:
        return rsa_decrypt( ctx->rsa.key, in, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, out, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST );
    default:
        SetLastError( NTE_BAD_ALGID );
        return FALSE;
    }
    return TRUE;
}

BOOL sign_hash_impl( KEY_CONTEXT *ctx, const BYTE *in, BYTE *out )
{
    return rsa_decrypt( ctx->rsa.key, in, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, out, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST );
}

BOOL verify_signature_impl( KEY_CONTEXT *ctx, const BYTE *in, BYTE *out )
{
    return rsa_encrypt( ctx->rsa.key, in, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, out, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST );
}

BOOL encrypt_stream_impl( ALG_ID algid, KEY_CONTEXT *ctx, BYTE *stream, DWORD len )
{
    switch (algid)
    {
    case CALG_RC4:
        SymCryptRc4Crypt( &ctx->rc4, stream, stream, len );
        break;
    default:
        SetLastError( NTE_BAD_ALGID );
        return FALSE;
    }
    return TRUE;
}

BOOL export_public_key_impl( const KEY_CONTEXT *ctx, BYTE *dst, DWORD *pubexp )
{
    BYTE *modulus = dst;
    SIZE_T modulus_size;
    UINT64 pubexp64;

    if (!ctx->rsa.key) return FALSE;

    modulus_size = SymCryptRsakeySizeofModulus( ctx->rsa.key );

    if (SymCryptRsakeyGetValue( ctx->rsa.key, modulus, modulus_size, &pubexp64, 1, NULL, NULL, 0,
                                SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, 0 ))
    {
        return FALSE;
        SetLastError( NTE_FAIL );
    }
    *pubexp = pubexp64;
    return TRUE;
}

BOOL import_public_key_impl( ALG_ID algid, const BYTE *src, DWORD keylen, DWORD pubexp, KEY_CONTEXT *ctx )
{
    const BYTE *modulus = src;
    UINT64 exp64 = pubexp;

    ctx->rsa.flags = SYMCRYPT_FLAG_KEY_NO_FIPS;
    if (algid == CALG_RSA_KEYX) ctx->rsa.flags |= SYMCRYPT_FLAG_RSAKEY_ENCRYPT;
    else ctx->rsa.flags |= SYMCRYPT_FLAG_RSAKEY_SIGN | SYMCRYPT_FLAG_RSAKEY_ENCRYPT;

    if (!(ctx->rsa.key = alloc_rsa_key( keylen * 8, FALSE )))
    {
        SetLastError( NTE_FAIL );
        return FALSE;
    }
    if (SymCryptRsakeySetValue( modulus, keylen, &exp64, 1, NULL, NULL, 0, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST,
                                ctx->rsa.flags, ctx->rsa.key ))
    {
        SetLastError( NTE_FAIL );
        return FALSE;
    }
    return TRUE;
}

BOOL export_private_key_impl( const KEY_CONTEXT *ctx, BYTE *dst, DWORD *pubexp )
{
    BYTE *modulus, *primes[2], *exponents[2], *coefficient, *private_exp;
    SIZE_T modulus_size, primes_sizes[2];
    UINT64 pubexp64;

    if (!ctx->rsa.key) return FALSE;

    modulus_size = SymCryptRsakeySizeofModulus( ctx->rsa.key );

    primes_sizes[0] = SymCryptRsakeySizeofPrime( ctx->rsa.key, 0 );
    primes_sizes[1] = SymCryptRsakeySizeofPrime( ctx->rsa.key, 1 );

    modulus = dst;
    primes[0] = modulus + modulus_size;
    primes[1] = modulus + modulus_size + primes_sizes[0];

    if (SymCryptRsakeyGetValue( ctx->rsa.key, modulus, modulus_size, &pubexp64, 1, primes, primes_sizes, 2,
                                SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, 0 ))
    {
        return FALSE;
        SetLastError( NTE_FAIL );
    }

    exponents[0] = primes[1]    + primes_sizes[1];
    exponents[1] = exponents[0] + primes_sizes[0];
    coefficient  = exponents[1] + primes_sizes[1];
    private_exp  = coefficient  + primes_sizes[0];

    if (SymCryptRsakeyGetCrtValue( ctx->rsa.key, exponents, primes_sizes, 2, coefficient, primes_sizes[0],
                                   private_exp, modulus_size, SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, 0 ))
    {
        return FALSE;
        SetLastError( NTE_FAIL );
    }

    *pubexp = pubexp64;
    return TRUE;
}

BOOL import_private_key_impl( ALG_ID algid, const BYTE *src, DWORD keylen, DWORD pubexp, KEY_CONTEXT *ctx )
{
    const BYTE *modulus = src, *primes[2];
    SIZE_T primes_sizes[2];
    UINT64 exp64 = pubexp;

    ctx->rsa.flags = SYMCRYPT_FLAG_KEY_NO_FIPS;
    if (algid == CALG_RSA_KEYX) ctx->rsa.flags |= SYMCRYPT_FLAG_RSAKEY_ENCRYPT;
    else ctx->rsa.flags |= SYMCRYPT_FLAG_RSAKEY_SIGN | SYMCRYPT_FLAG_RSAKEY_ENCRYPT;

    if (!(ctx->rsa.key = alloc_rsa_key( keylen * 8, TRUE )))
    {
        SetLastError( NTE_FAIL );
        return FALSE;
    }

    primes[0] = modulus + keylen;
    primes[1] = primes[0] + keylen / 2;

    primes_sizes[0] = keylen / 2;
    primes_sizes[1] = keylen / 2;

    if (SymCryptRsakeySetValue( modulus, keylen, &exp64, 1, primes, primes_sizes, 2,
                                SYMCRYPT_NUMBER_FORMAT_LSB_FIRST, ctx->rsa.flags, ctx->rsa.key ))
    {
        SetLastError( NTE_FAIL );
        return FALSE;
    }

    return TRUE;
}
