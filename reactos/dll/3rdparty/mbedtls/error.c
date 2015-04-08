/*
 *  Error message information
 *
 *  Copyright (C) 2006-2014, ARM Limited, All Rights Reserved
 *
 *  This file is part of mbed TLS (https://polarssl.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_ERROR_C) || defined(POLARSSL_ERROR_STRERROR_DUMMY)
#include "polarssl/error.h"
#endif

#if defined(POLARSSL_ERROR_C)

#if defined(POLARSSL_AES_C)
#include "polarssl/aes.h"
#endif

#if defined(POLARSSL_BASE64_C)
#include "polarssl/base64.h"
#endif

#if defined(POLARSSL_BIGNUM_C)
#include "polarssl/bignum.h"
#endif

#if defined(POLARSSL_BLOWFISH_C)
#include "polarssl/blowfish.h"
#endif

#if defined(POLARSSL_CAMELLIA_C)
#include "polarssl/camellia.h"
#endif

#if defined(POLARSSL_CCM_C)
#include "polarssl/ccm.h"
#endif

#if defined(POLARSSL_CIPHER_C)
#include "polarssl/cipher.h"
#endif

#if defined(POLARSSL_CTR_DRBG_C)
#include "polarssl/ctr_drbg.h"
#endif

#if defined(POLARSSL_DES_C)
#include "polarssl/des.h"
#endif

#if defined(POLARSSL_DHM_C)
#include "polarssl/dhm.h"
#endif

#if defined(POLARSSL_ECP_C)
#include "polarssl/ecp.h"
#endif

#if defined(POLARSSL_ENTROPY_C)
#include "polarssl/entropy.h"
#endif

#if defined(POLARSSL_GCM_C)
#include "polarssl/gcm.h"
#endif

#if defined(POLARSSL_HMAC_DRBG_C)
#include "polarssl/hmac_drbg.h"
#endif

#if defined(POLARSSL_MD_C)
#include "polarssl/md.h"
#endif

#if defined(POLARSSL_MD2_C)
#include "polarssl/md2.h"
#endif

#if defined(POLARSSL_MD4_C)
#include "polarssl/md4.h"
#endif

#if defined(POLARSSL_MD5_C)
#include "polarssl/md5.h"
#endif

#if defined(POLARSSL_NET_C)
#include "polarssl/net.h"
#endif

#if defined(POLARSSL_OID_C)
#include "polarssl/oid.h"
#endif

#if defined(POLARSSL_PADLOCK_C)
#include "polarssl/padlock.h"
#endif

#if defined(POLARSSL_PBKDF2_C)
#include "polarssl/pbkdf2.h"
#endif

#if defined(POLARSSL_PEM_PARSE_C) || defined(POLARSSL_PEM_WRITE_C)
#include "polarssl/pem.h"
#endif

#if defined(POLARSSL_PK_C)
#include "polarssl/pk.h"
#endif

#if defined(POLARSSL_PKCS12_C)
#include "polarssl/pkcs12.h"
#endif

#if defined(POLARSSL_PKCS5_C)
#include "polarssl/pkcs5.h"
#endif

#if defined(POLARSSL_RIPEMD160_C)
#include "polarssl/ripemd160.h"
#endif

#if defined(POLARSSL_RSA_C)
#include "polarssl/rsa.h"
#endif

#if defined(POLARSSL_SHA1_C)
#include "polarssl/sha1.h"
#endif

#if defined(POLARSSL_SHA256_C)
#include "polarssl/sha256.h"
#endif

#if defined(POLARSSL_SHA512_C)
#include "polarssl/sha512.h"
#endif

#if defined(POLARSSL_SSL_TLS_C)
#include "polarssl/ssl.h"
#endif

#if defined(POLARSSL_THREADING_C)
#include "polarssl/threading.h"
#endif

#if defined(POLARSSL_X509_USE_C) || defined(POLARSSL_X509_CREATE_C)
#include "polarssl/x509.h"
#endif

#if defined(POLARSSL_XTEA_C)
#include "polarssl/xtea.h"
#endif

#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER) && !defined  snprintf && !defined(EFIX64) && \
    !defined(EFI32)
#define  snprintf  _snprintf
#endif

void polarssl_strerror( int ret, char *buf, size_t buflen )
{
    size_t len;
    int use_ret;

    if( buflen == 0 )
        return;

    memset( buf, 0x00, buflen );
    /* Reduce buflen to make sure MSVC _snprintf() ends with \0 as well */
    buflen -= 1;

    if( ret < 0 )
        ret = -ret;

    if( ret & 0xFF80 )
    {
        use_ret = ret & 0xFF80;

        // High level error codes
        //
        // BEGIN generated code
#if defined(POLARSSL_CIPHER_C)
        if( use_ret == -(POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "CIPHER - The selected feature is not available" );
        if( use_ret == -(POLARSSL_ERR_CIPHER_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "CIPHER - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_CIPHER_ALLOC_FAILED) )
            snprintf( buf, buflen, "CIPHER - Failed to allocate memory" );
        if( use_ret == -(POLARSSL_ERR_CIPHER_INVALID_PADDING) )
            snprintf( buf, buflen, "CIPHER - Input data contains invalid padding and is rejected" );
        if( use_ret == -(POLARSSL_ERR_CIPHER_FULL_BLOCK_EXPECTED) )
            snprintf( buf, buflen, "CIPHER - Decryption of block requires a full block" );
        if( use_ret == -(POLARSSL_ERR_CIPHER_AUTH_FAILED) )
            snprintf( buf, buflen, "CIPHER - Authentication failed (for AEAD modes)" );
#endif /* POLARSSL_CIPHER_C */

#if defined(POLARSSL_DHM_C)
        if( use_ret == -(POLARSSL_ERR_DHM_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "DHM - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_DHM_READ_PARAMS_FAILED) )
            snprintf( buf, buflen, "DHM - Reading of the DHM parameters failed" );
        if( use_ret == -(POLARSSL_ERR_DHM_MAKE_PARAMS_FAILED) )
            snprintf( buf, buflen, "DHM - Making of the DHM parameters failed" );
        if( use_ret == -(POLARSSL_ERR_DHM_READ_PUBLIC_FAILED) )
            snprintf( buf, buflen, "DHM - Reading of the public values failed" );
        if( use_ret == -(POLARSSL_ERR_DHM_MAKE_PUBLIC_FAILED) )
            snprintf( buf, buflen, "DHM - Making of the public value failed" );
        if( use_ret == -(POLARSSL_ERR_DHM_CALC_SECRET_FAILED) )
            snprintf( buf, buflen, "DHM - Calculation of the DHM secret failed" );
        if( use_ret == -(POLARSSL_ERR_DHM_INVALID_FORMAT) )
            snprintf( buf, buflen, "DHM - The ASN.1 data is not formatted correctly" );
        if( use_ret == -(POLARSSL_ERR_DHM_MALLOC_FAILED) )
            snprintf( buf, buflen, "DHM - Allocation of memory failed" );
        if( use_ret == -(POLARSSL_ERR_DHM_FILE_IO_ERROR) )
            snprintf( buf, buflen, "DHM - Read/write of file failed" );
#endif /* POLARSSL_DHM_C */

#if defined(POLARSSL_ECP_C)
        if( use_ret == -(POLARSSL_ERR_ECP_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "ECP - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_ECP_BUFFER_TOO_SMALL) )
            snprintf( buf, buflen, "ECP - The buffer is too small to write to" );
        if( use_ret == -(POLARSSL_ERR_ECP_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "ECP - Requested curve not available" );
        if( use_ret == -(POLARSSL_ERR_ECP_VERIFY_FAILED) )
            snprintf( buf, buflen, "ECP - The signature is not valid" );
        if( use_ret == -(POLARSSL_ERR_ECP_MALLOC_FAILED) )
            snprintf( buf, buflen, "ECP - Memory allocation failed" );
        if( use_ret == -(POLARSSL_ERR_ECP_RANDOM_FAILED) )
            snprintf( buf, buflen, "ECP - Generation of random value, such as (ephemeral) key, failed" );
        if( use_ret == -(POLARSSL_ERR_ECP_INVALID_KEY) )
            snprintf( buf, buflen, "ECP - Invalid private or public key" );
        if( use_ret == -(POLARSSL_ERR_ECP_SIG_LEN_MISMATCH) )
            snprintf( buf, buflen, "ECP - Signature is valid but shorter than the user-supplied length" );
#endif /* POLARSSL_ECP_C */

#if defined(POLARSSL_MD_C)
        if( use_ret == -(POLARSSL_ERR_MD_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "MD - The selected feature is not available" );
        if( use_ret == -(POLARSSL_ERR_MD_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "MD - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_MD_ALLOC_FAILED) )
            snprintf( buf, buflen, "MD - Failed to allocate memory" );
        if( use_ret == -(POLARSSL_ERR_MD_FILE_IO_ERROR) )
            snprintf( buf, buflen, "MD - Opening or reading of file failed" );
#endif /* POLARSSL_MD_C */

#if defined(POLARSSL_PEM_PARSE_C) || defined(POLARSSL_PEM_WRITE_C)
        if( use_ret == -(POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT) )
            snprintf( buf, buflen, "PEM - No PEM header or footer found" );
        if( use_ret == -(POLARSSL_ERR_PEM_INVALID_DATA) )
            snprintf( buf, buflen, "PEM - PEM string is not as expected" );
        if( use_ret == -(POLARSSL_ERR_PEM_MALLOC_FAILED) )
            snprintf( buf, buflen, "PEM - Failed to allocate memory" );
        if( use_ret == -(POLARSSL_ERR_PEM_INVALID_ENC_IV) )
            snprintf( buf, buflen, "PEM - RSA IV is not in hex-format" );
        if( use_ret == -(POLARSSL_ERR_PEM_UNKNOWN_ENC_ALG) )
            snprintf( buf, buflen, "PEM - Unsupported key encryption algorithm" );
        if( use_ret == -(POLARSSL_ERR_PEM_PASSWORD_REQUIRED) )
            snprintf( buf, buflen, "PEM - Private key password can't be empty" );
        if( use_ret == -(POLARSSL_ERR_PEM_PASSWORD_MISMATCH) )
            snprintf( buf, buflen, "PEM - Given private key password does not allow for correct decryption" );
        if( use_ret == -(POLARSSL_ERR_PEM_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "PEM - Unavailable feature, e.g. hashing/encryption combination" );
        if( use_ret == -(POLARSSL_ERR_PEM_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "PEM - Bad input parameters to function" );
#endif /* POLARSSL_PEM_PARSE_C || POLARSSL_PEM_WRITE_C */

#if defined(POLARSSL_PK_C)
        if( use_ret == -(POLARSSL_ERR_PK_MALLOC_FAILED) )
            snprintf( buf, buflen, "PK - Memory alloation failed" );
        if( use_ret == -(POLARSSL_ERR_PK_TYPE_MISMATCH) )
            snprintf( buf, buflen, "PK - Type mismatch, eg attempt to encrypt with an ECDSA key" );
        if( use_ret == -(POLARSSL_ERR_PK_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "PK - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_PK_FILE_IO_ERROR) )
            snprintf( buf, buflen, "PK - Read/write of file failed" );
        if( use_ret == -(POLARSSL_ERR_PK_KEY_INVALID_VERSION) )
            snprintf( buf, buflen, "PK - Unsupported key version" );
        if( use_ret == -(POLARSSL_ERR_PK_KEY_INVALID_FORMAT) )
            snprintf( buf, buflen, "PK - Invalid key tag or value" );
        if( use_ret == -(POLARSSL_ERR_PK_UNKNOWN_PK_ALG) )
            snprintf( buf, buflen, "PK - Key algorithm is unsupported (only RSA and EC are supported)" );
        if( use_ret == -(POLARSSL_ERR_PK_PASSWORD_REQUIRED) )
            snprintf( buf, buflen, "PK - Private key password can't be empty" );
        if( use_ret == -(POLARSSL_ERR_PK_PASSWORD_MISMATCH) )
            snprintf( buf, buflen, "PK - Given private key password does not allow for correct decryption" );
        if( use_ret == -(POLARSSL_ERR_PK_INVALID_PUBKEY) )
            snprintf( buf, buflen, "PK - The pubkey tag or value is invalid (only RSA and EC are supported)" );
        if( use_ret == -(POLARSSL_ERR_PK_INVALID_ALG) )
            snprintf( buf, buflen, "PK - The algorithm tag or value is invalid" );
        if( use_ret == -(POLARSSL_ERR_PK_UNKNOWN_NAMED_CURVE) )
            snprintf( buf, buflen, "PK - Elliptic curve is unsupported (only NIST curves are supported)" );
        if( use_ret == -(POLARSSL_ERR_PK_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "PK - Unavailable feature, e.g. RSA disabled for RSA key" );
        if( use_ret == -(POLARSSL_ERR_PK_SIG_LEN_MISMATCH) )
            snprintf( buf, buflen, "PK - The signature is valid but its length is less than expected" );
#endif /* POLARSSL_PK_C */

#if defined(POLARSSL_PKCS12_C)
        if( use_ret == -(POLARSSL_ERR_PKCS12_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "PKCS12 - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_PKCS12_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "PKCS12 - Feature not available, e.g. unsupported encryption scheme" );
        if( use_ret == -(POLARSSL_ERR_PKCS12_PBE_INVALID_FORMAT) )
            snprintf( buf, buflen, "PKCS12 - PBE ASN.1 data not as expected" );
        if( use_ret == -(POLARSSL_ERR_PKCS12_PASSWORD_MISMATCH) )
            snprintf( buf, buflen, "PKCS12 - Given private key password does not allow for correct decryption" );
#endif /* POLARSSL_PKCS12_C */

#if defined(POLARSSL_PKCS5_C)
        if( use_ret == -(POLARSSL_ERR_PKCS5_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "PKCS5 - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_PKCS5_INVALID_FORMAT) )
            snprintf( buf, buflen, "PKCS5 - Unexpected ASN.1 data" );
        if( use_ret == -(POLARSSL_ERR_PKCS5_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "PKCS5 - Requested encryption or digest alg not available" );
        if( use_ret == -(POLARSSL_ERR_PKCS5_PASSWORD_MISMATCH) )
            snprintf( buf, buflen, "PKCS5 - Given private key password does not allow for correct decryption" );
#endif /* POLARSSL_PKCS5_C */

#if defined(POLARSSL_RSA_C)
        if( use_ret == -(POLARSSL_ERR_RSA_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "RSA - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_RSA_INVALID_PADDING) )
            snprintf( buf, buflen, "RSA - Input data contains invalid padding and is rejected" );
        if( use_ret == -(POLARSSL_ERR_RSA_KEY_GEN_FAILED) )
            snprintf( buf, buflen, "RSA - Something failed during generation of a key" );
        if( use_ret == -(POLARSSL_ERR_RSA_KEY_CHECK_FAILED) )
            snprintf( buf, buflen, "RSA - Key failed to pass the libraries validity check" );
        if( use_ret == -(POLARSSL_ERR_RSA_PUBLIC_FAILED) )
            snprintf( buf, buflen, "RSA - The public key operation failed" );
        if( use_ret == -(POLARSSL_ERR_RSA_PRIVATE_FAILED) )
            snprintf( buf, buflen, "RSA - The private key operation failed" );
        if( use_ret == -(POLARSSL_ERR_RSA_VERIFY_FAILED) )
            snprintf( buf, buflen, "RSA - The PKCS#1 verification failed" );
        if( use_ret == -(POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE) )
            snprintf( buf, buflen, "RSA - The output buffer for decryption is not large enough" );
        if( use_ret == -(POLARSSL_ERR_RSA_RNG_FAILED) )
            snprintf( buf, buflen, "RSA - The random generator failed to generate non-zeros" );
#endif /* POLARSSL_RSA_C */

#if defined(POLARSSL_SSL_TLS_C)
        if( use_ret == -(POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "SSL - The requested feature is not available" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "SSL - Bad input parameters to function" );
        if( use_ret == -(POLARSSL_ERR_SSL_INVALID_MAC) )
            snprintf( buf, buflen, "SSL - Verification of the message MAC failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_INVALID_RECORD) )
            snprintf( buf, buflen, "SSL - An invalid SSL record was received" );
        if( use_ret == -(POLARSSL_ERR_SSL_CONN_EOF) )
            snprintf( buf, buflen, "SSL - The connection indicated an EOF" );
        if( use_ret == -(POLARSSL_ERR_SSL_UNKNOWN_CIPHER) )
            snprintf( buf, buflen, "SSL - An unknown cipher was received" );
        if( use_ret == -(POLARSSL_ERR_SSL_NO_CIPHER_CHOSEN) )
            snprintf( buf, buflen, "SSL - The server has no ciphersuites in common with the client" );
        if( use_ret == -(POLARSSL_ERR_SSL_NO_RNG) )
            snprintf( buf, buflen, "SSL - No RNG was provided to the SSL module" );
        if( use_ret == -(POLARSSL_ERR_SSL_NO_CLIENT_CERTIFICATE) )
            snprintf( buf, buflen, "SSL - No client certification received from the client, but required by the authentication mode" );
        if( use_ret == -(POLARSSL_ERR_SSL_CERTIFICATE_TOO_LARGE) )
            snprintf( buf, buflen, "SSL - Our own certificate(s) is/are too large to send in an SSL message" );
        if( use_ret == -(POLARSSL_ERR_SSL_CERTIFICATE_REQUIRED) )
            snprintf( buf, buflen, "SSL - The own certificate is not set, but needed by the server" );
        if( use_ret == -(POLARSSL_ERR_SSL_PRIVATE_KEY_REQUIRED) )
            snprintf( buf, buflen, "SSL - The own private key or pre-shared key is not set, but needed" );
        if( use_ret == -(POLARSSL_ERR_SSL_CA_CHAIN_REQUIRED) )
            snprintf( buf, buflen, "SSL - No CA Chain is set, but required to operate" );
        if( use_ret == -(POLARSSL_ERR_SSL_UNEXPECTED_MESSAGE) )
            snprintf( buf, buflen, "SSL - An unexpected message was received from our peer" );
        if( use_ret == -(POLARSSL_ERR_SSL_FATAL_ALERT_MESSAGE) )
        {
            snprintf( buf, buflen, "SSL - A fatal alert message was received from our peer" );
            return;
        }
        if( use_ret == -(POLARSSL_ERR_SSL_PEER_VERIFY_FAILED) )
            snprintf( buf, buflen, "SSL - Verification of our peer failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY) )
            snprintf( buf, buflen, "SSL - The peer notified us that the connection is going to be closed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_CLIENT_HELLO) )
            snprintf( buf, buflen, "SSL - Processing of the ClientHello handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO) )
            snprintf( buf, buflen, "SSL - Processing of the ServerHello handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE) )
            snprintf( buf, buflen, "SSL - Processing of the Certificate handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE_REQUEST) )
            snprintf( buf, buflen, "SSL - Processing of the CertificateRequest handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_SERVER_KEY_EXCHANGE) )
            snprintf( buf, buflen, "SSL - Processing of the ServerKeyExchange handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_SERVER_HELLO_DONE) )
            snprintf( buf, buflen, "SSL - Processing of the ServerHelloDone handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE) )
            snprintf( buf, buflen, "SSL - Processing of the ClientKeyExchange handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_RP) )
            snprintf( buf, buflen, "SSL - Processing of the ClientKeyExchange handshake message failed in DHM / ECDH Read Public" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_CLIENT_KEY_EXCHANGE_CS) )
            snprintf( buf, buflen, "SSL - Processing of the ClientKeyExchange handshake message failed in DHM / ECDH Calculate Secret" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_CERTIFICATE_VERIFY) )
            snprintf( buf, buflen, "SSL - Processing of the CertificateVerify handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_CHANGE_CIPHER_SPEC) )
            snprintf( buf, buflen, "SSL - Processing of the ChangeCipherSpec handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_FINISHED) )
            snprintf( buf, buflen, "SSL - Processing of the Finished handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_MALLOC_FAILED) )
            snprintf( buf, buflen, "SSL - Memory allocation failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_HW_ACCEL_FAILED) )
            snprintf( buf, buflen, "SSL - Hardware acceleration function returned with error" );
        if( use_ret == -(POLARSSL_ERR_SSL_HW_ACCEL_FALLTHROUGH) )
            snprintf( buf, buflen, "SSL - Hardware acceleration function skipped / left alone data" );
        if( use_ret == -(POLARSSL_ERR_SSL_COMPRESSION_FAILED) )
            snprintf( buf, buflen, "SSL - Processing of the compression / decompression failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_PROTOCOL_VERSION) )
            snprintf( buf, buflen, "SSL - Handshake protocol not within min/max boundaries" );
        if( use_ret == -(POLARSSL_ERR_SSL_BAD_HS_NEW_SESSION_TICKET) )
            snprintf( buf, buflen, "SSL - Processing of the NewSessionTicket handshake message failed" );
        if( use_ret == -(POLARSSL_ERR_SSL_SESSION_TICKET_EXPIRED) )
            snprintf( buf, buflen, "SSL - Session ticket has expired" );
        if( use_ret == -(POLARSSL_ERR_SSL_PK_TYPE_MISMATCH) )
            snprintf( buf, buflen, "SSL - Public key type mismatch (eg, asked for RSA key exchange and presented EC key)" );
        if( use_ret == -(POLARSSL_ERR_SSL_UNKNOWN_IDENTITY) )
            snprintf( buf, buflen, "SSL - Unknown identity received (eg, PSK identity)" );
        if( use_ret == -(POLARSSL_ERR_SSL_INTERNAL_ERROR) )
            snprintf( buf, buflen, "SSL - Internal error (eg, unexpected failure in lower-level module)" );
        if( use_ret == -(POLARSSL_ERR_SSL_COUNTER_WRAPPING) )
            snprintf( buf, buflen, "SSL - A counter would wrap (eg, too many messages exchanged)" );
        if( use_ret == -(POLARSSL_ERR_SSL_WAITING_SERVER_HELLO_RENEGO) )
            snprintf( buf, buflen, "SSL - Unexpected message at ServerHello in renegotiation" );
        if( use_ret == -(POLARSSL_ERR_SSL_NO_USABLE_CIPHERSUITE) )
            snprintf( buf, buflen, "SSL - None of the common ciphersuites is usable (eg, no suitable certificate, see debug messages)" );
#endif /* POLARSSL_SSL_TLS_C */

#if defined(POLARSSL_X509_USE_C) || defined(POLARSSL_X509_CREATE_C)
        if( use_ret == -(POLARSSL_ERR_X509_FEATURE_UNAVAILABLE) )
            snprintf( buf, buflen, "X509 - Unavailable feature, e.g. RSA hashing/encryption combination" );
        if( use_ret == -(POLARSSL_ERR_X509_UNKNOWN_OID) )
            snprintf( buf, buflen, "X509 - Requested OID is unknown" );
        if( use_ret == -(POLARSSL_ERR_X509_INVALID_FORMAT) )
            snprintf( buf, buflen, "X509 - The CRT/CRL/CSR format is invalid, e.g. different type expected" );
        if( use_ret == -(POLARSSL_ERR_X509_INVALID_VERSION) )
            snprintf( buf, buflen, "X509 - The CRT/CRL/CSR version element is invalid" );
        if( use_ret == -(POLARSSL_ERR_X509_INVALID_SERIAL) )
            snprintf( buf, buflen, "X509 - The serial tag or value is invalid" );
        if( use_ret == -(POLARSSL_ERR_X509_INVALID_ALG) )
            snprintf( buf, buflen, "X509 - The algorithm tag or value is invalid" );
        if( use_ret == -(POLARSSL_ERR_X509_INVALID_NAME) )
            snprintf( buf, buflen, "X509 - The name tag or value is invalid" );
        if( use_ret == -(POLARSSL_ERR_X509_INVALID_DATE) )
            snprintf( buf, buflen, "X509 - The date tag or value is invalid" );
        if( use_ret == -(POLARSSL_ERR_X509_INVALID_SIGNATURE) )
            snprintf( buf, buflen, "X509 - The signature tag or value invalid" );
        if( use_ret == -(POLARSSL_ERR_X509_INVALID_EXTENSIONS) )
            snprintf( buf, buflen, "X509 - The extension tag or value is invalid" );
        if( use_ret == -(POLARSSL_ERR_X509_UNKNOWN_VERSION) )
            snprintf( buf, buflen, "X509 - CRT/CRL/CSR has an unsupported version number" );
        if( use_ret == -(POLARSSL_ERR_X509_UNKNOWN_SIG_ALG) )
            snprintf( buf, buflen, "X509 - Signature algorithm (oid) is unsupported" );
        if( use_ret == -(POLARSSL_ERR_X509_SIG_MISMATCH) )
            snprintf( buf, buflen, "X509 - Signature algorithms do not match. (see \\c ::x509_crt sig_oid)" );
        if( use_ret == -(POLARSSL_ERR_X509_CERT_VERIFY_FAILED) )
            snprintf( buf, buflen, "X509 - Certificate verification failed, e.g. CRL, CA or signature check failed" );
        if( use_ret == -(POLARSSL_ERR_X509_CERT_UNKNOWN_FORMAT) )
            snprintf( buf, buflen, "X509 - Format not recognized as DER or PEM" );
        if( use_ret == -(POLARSSL_ERR_X509_BAD_INPUT_DATA) )
            snprintf( buf, buflen, "X509 - Input invalid" );
        if( use_ret == -(POLARSSL_ERR_X509_MALLOC_FAILED) )
            snprintf( buf, buflen, "X509 - Allocation of memory failed" );
        if( use_ret == -(POLARSSL_ERR_X509_FILE_IO_ERROR) )
            snprintf( buf, buflen, "X509 - Read/write of file failed" );
#endif /* POLARSSL_X509_USE,X509_CREATE_C */
        // END generated code

        if( strlen( buf ) == 0 )
            snprintf( buf, buflen, "UNKNOWN ERROR CODE (%04X)", use_ret );
    }

    use_ret = ret & ~0xFF80;

    if( use_ret == 0 )
        return;

    // If high level code is present, make a concatenation between both
    // error strings.
    //
    len = strlen( buf );

    if( len > 0 )
    {
        if( buflen - len < 5 )
            return;

        snprintf( buf + len, buflen - len, " : " );

        buf += len + 3;
        buflen -= len + 3;
    }

    // Low level error codes
    //
    // BEGIN generated code
#if defined(POLARSSL_AES_C)
    if( use_ret == -(POLARSSL_ERR_AES_INVALID_KEY_LENGTH) )
        snprintf( buf, buflen, "AES - Invalid key length" );
    if( use_ret == -(POLARSSL_ERR_AES_INVALID_INPUT_LENGTH) )
        snprintf( buf, buflen, "AES - Invalid data input length" );
#endif /* POLARSSL_AES_C */

#if defined(POLARSSL_ASN1_PARSE_C)
    if( use_ret == -(POLARSSL_ERR_ASN1_OUT_OF_DATA) )
        snprintf( buf, buflen, "ASN1 - Out of data when parsing an ASN1 data structure" );
    if( use_ret == -(POLARSSL_ERR_ASN1_UNEXPECTED_TAG) )
        snprintf( buf, buflen, "ASN1 - ASN1 tag was of an unexpected value" );
    if( use_ret == -(POLARSSL_ERR_ASN1_INVALID_LENGTH) )
        snprintf( buf, buflen, "ASN1 - Error when trying to determine the length or invalid length" );
    if( use_ret == -(POLARSSL_ERR_ASN1_LENGTH_MISMATCH) )
        snprintf( buf, buflen, "ASN1 - Actual length differs from expected length" );
    if( use_ret == -(POLARSSL_ERR_ASN1_INVALID_DATA) )
        snprintf( buf, buflen, "ASN1 - Data is invalid. (not used)" );
    if( use_ret == -(POLARSSL_ERR_ASN1_MALLOC_FAILED) )
        snprintf( buf, buflen, "ASN1 - Memory allocation failed" );
    if( use_ret == -(POLARSSL_ERR_ASN1_BUF_TOO_SMALL) )
        snprintf( buf, buflen, "ASN1 - Buffer too small when writing ASN.1 data structure" );
#endif /* POLARSSL_ASN1_PARSE_C */

#if defined(POLARSSL_BASE64_C)
    if( use_ret == -(POLARSSL_ERR_BASE64_BUFFER_TOO_SMALL) )
        snprintf( buf, buflen, "BASE64 - Output buffer too small" );
    if( use_ret == -(POLARSSL_ERR_BASE64_INVALID_CHARACTER) )
        snprintf( buf, buflen, "BASE64 - Invalid character in input" );
#endif /* POLARSSL_BASE64_C */

#if defined(POLARSSL_BIGNUM_C)
    if( use_ret == -(POLARSSL_ERR_MPI_FILE_IO_ERROR) )
        snprintf( buf, buflen, "BIGNUM - An error occurred while reading from or writing to a file" );
    if( use_ret == -(POLARSSL_ERR_MPI_BAD_INPUT_DATA) )
        snprintf( buf, buflen, "BIGNUM - Bad input parameters to function" );
    if( use_ret == -(POLARSSL_ERR_MPI_INVALID_CHARACTER) )
        snprintf( buf, buflen, "BIGNUM - There is an invalid character in the digit string" );
    if( use_ret == -(POLARSSL_ERR_MPI_BUFFER_TOO_SMALL) )
        snprintf( buf, buflen, "BIGNUM - The buffer is too small to write to" );
    if( use_ret == -(POLARSSL_ERR_MPI_NEGATIVE_VALUE) )
        snprintf( buf, buflen, "BIGNUM - The input arguments are negative or result in illegal output" );
    if( use_ret == -(POLARSSL_ERR_MPI_DIVISION_BY_ZERO) )
        snprintf( buf, buflen, "BIGNUM - The input argument for division is zero, which is not allowed" );
    if( use_ret == -(POLARSSL_ERR_MPI_NOT_ACCEPTABLE) )
        snprintf( buf, buflen, "BIGNUM - The input arguments are not acceptable" );
    if( use_ret == -(POLARSSL_ERR_MPI_MALLOC_FAILED) )
        snprintf( buf, buflen, "BIGNUM - Memory allocation failed" );
#endif /* POLARSSL_BIGNUM_C */

#if defined(POLARSSL_BLOWFISH_C)
    if( use_ret == -(POLARSSL_ERR_BLOWFISH_INVALID_KEY_LENGTH) )
        snprintf( buf, buflen, "BLOWFISH - Invalid key length" );
    if( use_ret == -(POLARSSL_ERR_BLOWFISH_INVALID_INPUT_LENGTH) )
        snprintf( buf, buflen, "BLOWFISH - Invalid data input length" );
#endif /* POLARSSL_BLOWFISH_C */

#if defined(POLARSSL_CAMELLIA_C)
    if( use_ret == -(POLARSSL_ERR_CAMELLIA_INVALID_KEY_LENGTH) )
        snprintf( buf, buflen, "CAMELLIA - Invalid key length" );
    if( use_ret == -(POLARSSL_ERR_CAMELLIA_INVALID_INPUT_LENGTH) )
        snprintf( buf, buflen, "CAMELLIA - Invalid data input length" );
#endif /* POLARSSL_CAMELLIA_C */

#if defined(POLARSSL_CCM_C)
    if( use_ret == -(POLARSSL_ERR_CCM_BAD_INPUT) )
        snprintf( buf, buflen, "CCM - Bad input parameters to function" );
    if( use_ret == -(POLARSSL_ERR_CCM_AUTH_FAILED) )
        snprintf( buf, buflen, "CCM - Authenticated decryption failed" );
#endif /* POLARSSL_CCM_C */

#if defined(POLARSSL_CTR_DRBG_C)
    if( use_ret == -(POLARSSL_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED) )
        snprintf( buf, buflen, "CTR_DRBG - The entropy source failed" );
    if( use_ret == -(POLARSSL_ERR_CTR_DRBG_REQUEST_TOO_BIG) )
        snprintf( buf, buflen, "CTR_DRBG - Too many random requested in single call" );
    if( use_ret == -(POLARSSL_ERR_CTR_DRBG_INPUT_TOO_BIG) )
        snprintf( buf, buflen, "CTR_DRBG - Input too large (Entropy + additional)" );
    if( use_ret == -(POLARSSL_ERR_CTR_DRBG_FILE_IO_ERROR) )
        snprintf( buf, buflen, "CTR_DRBG - Read/write error in file" );
#endif /* POLARSSL_CTR_DRBG_C */

#if defined(POLARSSL_DES_C)
    if( use_ret == -(POLARSSL_ERR_DES_INVALID_INPUT_LENGTH) )
        snprintf( buf, buflen, "DES - The data input has an invalid length" );
#endif /* POLARSSL_DES_C */

#if defined(POLARSSL_ENTROPY_C)
    if( use_ret == -(POLARSSL_ERR_ENTROPY_SOURCE_FAILED) )
        snprintf( buf, buflen, "ENTROPY - Critical entropy source failure" );
    if( use_ret == -(POLARSSL_ERR_ENTROPY_MAX_SOURCES) )
        snprintf( buf, buflen, "ENTROPY - No more sources can be added" );
    if( use_ret == -(POLARSSL_ERR_ENTROPY_NO_SOURCES_DEFINED) )
        snprintf( buf, buflen, "ENTROPY - No sources have been added to poll" );
    if( use_ret == -(POLARSSL_ERR_ENTROPY_FILE_IO_ERROR) )
        snprintf( buf, buflen, "ENTROPY - Read/write error in file" );
#endif /* POLARSSL_ENTROPY_C */

#if defined(POLARSSL_GCM_C)
    if( use_ret == -(POLARSSL_ERR_GCM_AUTH_FAILED) )
        snprintf( buf, buflen, "GCM - Authenticated decryption failed" );
    if( use_ret == -(POLARSSL_ERR_GCM_BAD_INPUT) )
        snprintf( buf, buflen, "GCM - Bad input parameters to function" );
#endif /* POLARSSL_GCM_C */

#if defined(POLARSSL_HMAC_DRBG_C)
    if( use_ret == -(POLARSSL_ERR_HMAC_DRBG_REQUEST_TOO_BIG) )
        snprintf( buf, buflen, "HMAC_DRBG - Too many random requested in single call" );
    if( use_ret == -(POLARSSL_ERR_HMAC_DRBG_INPUT_TOO_BIG) )
        snprintf( buf, buflen, "HMAC_DRBG - Input too large (Entropy + additional)" );
    if( use_ret == -(POLARSSL_ERR_HMAC_DRBG_FILE_IO_ERROR) )
        snprintf( buf, buflen, "HMAC_DRBG - Read/write error in file" );
    if( use_ret == -(POLARSSL_ERR_HMAC_DRBG_ENTROPY_SOURCE_FAILED) )
        snprintf( buf, buflen, "HMAC_DRBG - The entropy source failed" );
#endif /* POLARSSL_HMAC_DRBG_C */

#if defined(POLARSSL_MD2_C)
    if( use_ret == -(POLARSSL_ERR_MD2_FILE_IO_ERROR) )
        snprintf( buf, buflen, "MD2 - Read/write error in file" );
#endif /* POLARSSL_MD2_C */

#if defined(POLARSSL_MD4_C)
    if( use_ret == -(POLARSSL_ERR_MD4_FILE_IO_ERROR) )
        snprintf( buf, buflen, "MD4 - Read/write error in file" );
#endif /* POLARSSL_MD4_C */

#if defined(POLARSSL_MD5_C)
    if( use_ret == -(POLARSSL_ERR_MD5_FILE_IO_ERROR) )
        snprintf( buf, buflen, "MD5 - Read/write error in file" );
#endif /* POLARSSL_MD5_C */

#if defined(POLARSSL_NET_C)
    if( use_ret == -(POLARSSL_ERR_NET_UNKNOWN_HOST) )
        snprintf( buf, buflen, "NET - Failed to get an IP address for the given hostname" );
    if( use_ret == -(POLARSSL_ERR_NET_SOCKET_FAILED) )
        snprintf( buf, buflen, "NET - Failed to open a socket" );
    if( use_ret == -(POLARSSL_ERR_NET_CONNECT_FAILED) )
        snprintf( buf, buflen, "NET - The connection to the given server / port failed" );
    if( use_ret == -(POLARSSL_ERR_NET_BIND_FAILED) )
        snprintf( buf, buflen, "NET - Binding of the socket failed" );
    if( use_ret == -(POLARSSL_ERR_NET_LISTEN_FAILED) )
        snprintf( buf, buflen, "NET - Could not listen on the socket" );
    if( use_ret == -(POLARSSL_ERR_NET_ACCEPT_FAILED) )
        snprintf( buf, buflen, "NET - Could not accept the incoming connection" );
    if( use_ret == -(POLARSSL_ERR_NET_RECV_FAILED) )
        snprintf( buf, buflen, "NET - Reading information from the socket failed" );
    if( use_ret == -(POLARSSL_ERR_NET_SEND_FAILED) )
        snprintf( buf, buflen, "NET - Sending information through the socket failed" );
    if( use_ret == -(POLARSSL_ERR_NET_CONN_RESET) )
        snprintf( buf, buflen, "NET - Connection was reset by peer" );
    if( use_ret == -(POLARSSL_ERR_NET_WANT_READ) )
        snprintf( buf, buflen, "NET - Connection requires a read call" );
    if( use_ret == -(POLARSSL_ERR_NET_WANT_WRITE) )
        snprintf( buf, buflen, "NET - Connection requires a write call" );
#endif /* POLARSSL_NET_C */

#if defined(POLARSSL_OID_C)
    if( use_ret == -(POLARSSL_ERR_OID_NOT_FOUND) )
        snprintf( buf, buflen, "OID - OID is not found" );
    if( use_ret == -(POLARSSL_ERR_OID_BUF_TOO_SMALL) )
        snprintf( buf, buflen, "OID - output buffer is too small" );
#endif /* POLARSSL_OID_C */

#if defined(POLARSSL_PADLOCK_C)
    if( use_ret == -(POLARSSL_ERR_PADLOCK_DATA_MISALIGNED) )
        snprintf( buf, buflen, "PADLOCK - Input data should be aligned" );
#endif /* POLARSSL_PADLOCK_C */

#if defined(POLARSSL_PBKDF2_C)
    if( use_ret == -(POLARSSL_ERR_PBKDF2_BAD_INPUT_DATA) )
        snprintf( buf, buflen, "PBKDF2 - Bad input parameters to function" );
#endif /* POLARSSL_PBKDF2_C */

#if defined(POLARSSL_RIPEMD160_C)
    if( use_ret == -(POLARSSL_ERR_RIPEMD160_FILE_IO_ERROR) )
        snprintf( buf, buflen, "RIPEMD160 - Read/write error in file" );
#endif /* POLARSSL_RIPEMD160_C */

#if defined(POLARSSL_SHA1_C)
    if( use_ret == -(POLARSSL_ERR_SHA1_FILE_IO_ERROR) )
        snprintf( buf, buflen, "SHA1 - Read/write error in file" );
#endif /* POLARSSL_SHA1_C */

#if defined(POLARSSL_SHA256_C)
    if( use_ret == -(POLARSSL_ERR_SHA256_FILE_IO_ERROR) )
        snprintf( buf, buflen, "SHA256 - Read/write error in file" );
#endif /* POLARSSL_SHA256_C */

#if defined(POLARSSL_SHA512_C)
    if( use_ret == -(POLARSSL_ERR_SHA512_FILE_IO_ERROR) )
        snprintf( buf, buflen, "SHA512 - Read/write error in file" );
#endif /* POLARSSL_SHA512_C */

#if defined(POLARSSL_THREADING_C)
    if( use_ret == -(POLARSSL_ERR_THREADING_FEATURE_UNAVAILABLE) )
        snprintf( buf, buflen, "THREADING - The selected feature is not available" );
    if( use_ret == -(POLARSSL_ERR_THREADING_BAD_INPUT_DATA) )
        snprintf( buf, buflen, "THREADING - Bad input parameters to function" );
    if( use_ret == -(POLARSSL_ERR_THREADING_MUTEX_ERROR) )
        snprintf( buf, buflen, "THREADING - Locking / unlocking / free failed with error code" );
#endif /* POLARSSL_THREADING_C */

#if defined(POLARSSL_XTEA_C)
    if( use_ret == -(POLARSSL_ERR_XTEA_INVALID_INPUT_LENGTH) )
        snprintf( buf, buflen, "XTEA - The data input has an invalid length" );
#endif /* POLARSSL_XTEA_C */
    // END generated code

    if( strlen( buf ) != 0 )
        return;

    snprintf( buf, buflen, "UNKNOWN ERROR CODE (%04X)", use_ret );
}

#if defined(POLARSSL_ERROR_STRERROR_BC)
void error_strerror( int ret, char *buf, size_t buflen )
{
    polarssl_strerror( ret, buf, buflen );
}
#endif /* POLARSSL_ERROR_STRERROR_BC */

#else /* POLARSSL_ERROR_C */

#if defined(POLARSSL_ERROR_STRERROR_DUMMY)

#include <string.h>

/*
 * Provide an non-function in case POLARSSL_ERROR_C is not defined
 */
void polarssl_strerror( int ret, char *buf, size_t buflen )
{
    ((void) ret);

    if( buflen > 0 )
        buf[0] = '\0';
}

#if defined(POLARSSL_ERROR_STRERROR_BC)
void error_strerror( int ret, char *buf, size_t buflen )
{
    polarssl_strerror( ret, buf, buflen );
}
#endif /* POLARSSL_ERROR_STRERROR_BC */
#endif /* POLARSSL_ERROR_STRERROR_DUMMY */

#endif /* POLARSSL_ERROR_C */
