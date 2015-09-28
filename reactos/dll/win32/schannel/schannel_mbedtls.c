/*
 * Lightweight mbedTLS-based implementation of the schannel (SSL/TLS) provider.
 *
 * Copyright 2015 Peter Hater
 * Copyright 2015 Ismael Ferreras Morezuelas <swyterzone+ros@gmail.com>
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

#ifdef __REACTOS__
 #include "precomp.h"
#else
 #include <stdarg.h>
 #include <errno.h>

 #include "windef.h"
 #include "winbase.h"
 #include "sspi.h"
 #include "schannel.h"
 #include "wine/debug.h"
 #include "wine/library.h"
#endif

#if defined(SONAME_LIBMBEDTLS) && !defined(HAVE_SECURITY_SECURITY_H) && !defined(SONAME_LIBGNUTLS)

#include <polarssl/ssl.h>
#include <polarssl/entropy.h>
#include <polarssl/ctr_drbg.h>

#define ROS_SCHAN_IS_BLOCKING (0xCCCFFFFF & 0xFFF00000)
#define ROS_SCHAN_IS_BLOCKING_MARSHALL(read_len) (ROS_SCHAN_IS_BLOCKING | (read_len & 0x000FFFFF))
#define ROS_SCHAN_IS_BLOCKING_RETRIEVE(read_len)                          (read_len & 0x000FFFFF)

#ifndef __REACTOS__
 /* WINE defines the back-end glue in here */
 #include "secur32_priv.h"

 /* in ReactOS we use schannel instead of secur32 */
 WINE_DEFAULT_DEBUG_CHANNEL(secur32);

 /* WINE prefers to keep it optional, disable this to link explicitly */
 #include "schannel_mbedtls_lazyload.h"

 /* WINE does not define this standard win32 macro for some reason */
 #ifndef _countof
  #define _countof(a) (sizeof(a)/sizeof(*(a)))
 #endif
#endif

typedef struct
{
    ssl_context ssl;
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    struct schan_transport *transport;
} POLARSSL_SESSION, *PPOLARSSL_SESSION;

/* custom `net_recv` callback adapter, mbedTLS uses it in ssl_read for
   pulling data from the underlying win32 net stack */
static int schan_pull_adapter(void *session, unsigned char *buff, size_t buff_len)
{
    POLARSSL_SESSION *s = session;
    size_t requested = buff_len;
    int status;

    TRACE("POLARSSL schan_pull_adapter: (%p/%p, %p, %u)\n", s, s->transport, buff, buff_len);

    status = schan_pull(s->transport, buff, &buff_len);

    TRACE("POLARSSL schan_pull_adapter: (%p/%p, %p, %u) status: %#x\n", s, s->transport, buff, buff_len, status);

    if (status == NO_ERROR)
    {
        /* great, no more data left */
        if (buff_len == 0)
        {
            TRACE("Connection closed\n");
            return 0;
        }
        /* there's still some bytes that need pulling */
        else if (buff_len < requested)
        {
            TRACE("Pulled %u bytes before would block\n", buff_len);
            return ROS_SCHAN_IS_BLOCKING_MARSHALL(buff_len);
        }
        else
        {
            TRACE("Pulled %u bytes\n", buff_len);
            return buff_len;
        }
    }
    else if (status == EAGAIN)
    {
        TRACE("Would block before being able to pull anything, passing buff_len=%u\n", buff_len);
        return ROS_SCHAN_IS_BLOCKING_MARSHALL(buff_len);
    }
    else
    {
        ERR("Unknown status code from schan_pull: %d\n", status);
        return POLARSSL_ERR_NET_RECV_FAILED;
    }

    /* this should be unreachable */
    return POLARSSL_ERR_NET_CONNECT_FAILED;
}

/* custom `net_send` callback adapter, mbedTLS uses it in ssl_write for
   pushing data to the underlying win32 net stack */
static int schan_push_adapter(void *session, const unsigned char *buff, size_t buff_len)
{
    POLARSSL_SESSION *s = session;
    int status;

    TRACE("POLARSSL schan_push_adapter: (%p/%p, %p, %u)\n", s, s->transport, buff, buff_len);

    status = schan_push(s->transport, buff, &buff_len);

    TRACE("POLARSSL schan_push_adapter: (%p/%p, %p, %u) status: %#x\n", s, s->transport, buff, buff_len, status);

    if (status == NO_ERROR)
    {
        TRACE("Pushed %u bytes\n", buff_len);
        return buff_len;
    }
    else if (status == EAGAIN)
    {
        TRACE("Would block before being able to push anything. passing %u\n", buff_len);
        return ROS_SCHAN_IS_BLOCKING_MARSHALL(buff_len);
    }
    else
    {
        ERR("Unknown status code from schan_push: %d\n", status);
        return POLARSSL_ERR_NET_SEND_FAILED;
    }

    /* this should be unreachable */
    return POLARSSL_ERR_NET_CONNECT_FAILED;
}

DWORD schan_imp_enabled_protocols(void)
{
    /* NOTE: No support for SSL 2.0 */
    TRACE("POLARSSL schan_imp_enabled_protocols()\n");

    return 0
#ifdef POLARSSL_SSL_PROTO_SSL3
        | SP_PROT_SSL3_CLIENT | SP_PROT_SSL3_SERVER
#endif
#ifdef POLARSSL_SSL_PROTO_TLS1
        | SP_PROT_TLS1_0_CLIENT | SP_PROT_TLS1_0_SERVER
#endif
#ifdef POLARSSL_SSL_PROTO_TLS1_1
        | SP_PROT_TLS1_1_CLIENT | SP_PROT_TLS1_1_SERVER
#endif
#ifdef POLARSSL_SSL_PROTO_TLS1_2
        | SP_PROT_TLS1_2_CLIENT | SP_PROT_TLS1_2_SERVER
#endif
        ;
}

BOOL schan_imp_create_session(schan_imp_session *session, schan_credentials *cred)
{
    POLARSSL_SESSION *s;
    int ret;

    WARN("POLARSSL schan_imp_create_session: %p %p %p\n", session, *session, cred);

    s = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POLARSSL_SESSION));

    if (!(*session = (schan_imp_session)s))
    {
        ERR("Not enough memory to create session\n");
        return FALSE;
    }

    TRACE("POLARSSL init entropy\n");
    entropy_init(&s->entropy);

    TRACE("POLARSSL init random - change static entropy private data\n");
    ret = ctr_drbg_init(&s->ctr_drbg, entropy_func, &s->entropy, (unsigned char *) "PolarSSL", 8);

    if (ret != 0)
    {
        ERR("ctr_drbg_init failed with -%x\n", -ret);

        entropy_free(&s->entropy);

        /* safely overwrite the freed context with zeroes */
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, s);

        return FALSE;
    }

    WARN("POLARSSL init ssl\n");
    ret = ssl_init(&s->ssl);

    if (ret != 0)
    {
        ERR("Error SSL initialization -%x.\n", -ret);

        ctr_drbg_free(&s->ctr_drbg);
        entropy_free(&s->entropy);

        /* safely overwrite the freed context with zeroes */
        HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, s);

        return FALSE;
    }

    TRACE("POLARSSL set BIO callbacks\n");
    ssl_set_bio(&s->ssl, schan_pull_adapter, s, schan_push_adapter, s);

    TRACE("POLARSSL set endpoint %d\n", cred->credential_use);
    ssl_set_endpoint(&s->ssl, (cred->credential_use & SECPKG_CRED_INBOUND) ? SSL_IS_SERVER : SSL_IS_CLIENT);

    TRACE("POLARSSL set authmode\n");
    ssl_set_authmode(&s->ssl, SSL_VERIFY_NONE);

    TRACE("POLARSSL set rng\n");
    ssl_set_rng(&s->ssl, ctr_drbg_random, &s->ctr_drbg);

    TRACE("POLARSSL set versions\n");
    ssl_set_min_version(&s->ssl, SSL_MIN_MAJOR_VERSION, SSL_MIN_MINOR_VERSION);
    ssl_set_max_version(&s->ssl, SSL_MAX_MAJOR_VERSION, SSL_MAX_MINOR_VERSION);

    TRACE("POLARSSL schan_imp_create_session END!\n");

    return TRUE;
}

void schan_imp_dispose_session(schan_imp_session session)
{
    POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;
    WARN("POLARSSL schan_imp_dispose_session: %p\n", session);

    /* tell the other peer (a server) that we are going away */
    //ssl_close_notify(&s->ssl);

    ssl_free(&s->ssl);
    ctr_drbg_free(&s->ctr_drbg);
    entropy_free(&s->entropy);

    /* safely overwrite the freed context with zeroes */
    HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, s);
}

void schan_imp_set_session_transport(schan_imp_session session,
                                     struct schan_transport *t)
{
    POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;

    TRACE("POLARSSL schan_imp_set_session_transport: %p %p %d\n", session, t, s->ssl.state);

    s->transport = t;
}

void schan_imp_set_session_target(schan_imp_session session, const char *target)
{
    POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;

    FIXME("POLARSSL schan_imp_set_session_target: sess: %p hostname: %s state: %d\n", session, target, s->ssl.state);

    /* FIXME: The wine tests do not pass when we set the hostname. */
    ssl_set_hostname(&s->ssl, target);
}

SECURITY_STATUS schan_imp_handshake(schan_imp_session session)
{
    POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;

    int err = ssl_handshake(&s->ssl);

    TRACE("POLARSSL schan_imp_handshake: %p %d  err: %#x \n", session, s->ssl.state, err);

    if ((err & ROS_SCHAN_IS_BLOCKING) == ROS_SCHAN_IS_BLOCKING)
    {
        TRACE("Received ERR_NET_WANT_READ/WRITE... let's try again!\n");
        return SEC_I_CONTINUE_NEEDED;
    }
    else if (err == POLARSSL_ERR_SSL_FEATURE_UNAVAILABLE)
    {
        ERR("schan_imp_handshake: SSL Feature unavailable...\n");
        return SEC_E_UNSUPPORTED_FUNCTION;
    }
    else if (err != 0)
    {
        ERR("schan_imp_handshake: Oops! ssl_handshake returned the following error code: -%#x...\n", -err);
        return SEC_E_INTERNAL_ERROR;
    }

    WARN("schan_imp_handshake: Handshake completed!\n");
    WARN("schan_imp_handshake: Protocol is %s, Cipher suite is %s\n", ssl_get_version(&s->ssl), ssl_get_ciphersuite(&s->ssl));
    return SEC_E_OK;
}

static unsigned int schannel_get_cipher_block_size(int ciphersuite_id)
{
    const ssl_ciphersuite_t *cipher_suite = ssl_ciphersuite_from_id(ciphersuite_id);
    const struct
    {
        int algo;
        unsigned int size;
    }
    algorithms[] =
    {
        {POLARSSL_CIPHER_NONE,                 1},
        {POLARSSL_CIPHER_NULL,                 1},

    #ifdef POLARSSL_AES_C
        {POLARSSL_CIPHER_AES_128_ECB,         16},
        {POLARSSL_CIPHER_AES_192_ECB,         16},
        {POLARSSL_CIPHER_AES_256_ECB,         16},
        {POLARSSL_CIPHER_AES_128_CBC,         16},
        {POLARSSL_CIPHER_AES_192_CBC,         16},
        {POLARSSL_CIPHER_AES_256_CBC,         16},
        {POLARSSL_CIPHER_AES_128_CFB128,      16},
        {POLARSSL_CIPHER_AES_192_CFB128,      16},
        {POLARSSL_CIPHER_AES_256_CFB128,      16},
        {POLARSSL_CIPHER_AES_128_CTR,         16},
        {POLARSSL_CIPHER_AES_192_CTR,         16},
        {POLARSSL_CIPHER_AES_256_CTR,         16},
        {POLARSSL_CIPHER_AES_128_GCM,         16},
        {POLARSSL_CIPHER_AES_192_GCM,         16},
        {POLARSSL_CIPHER_AES_256_GCM,         16},
    #endif

    #ifdef POLARSSL_CAMELLIA_C
        {POLARSSL_CIPHER_CAMELLIA_128_ECB,    16},
        {POLARSSL_CIPHER_CAMELLIA_192_ECB,    16},
        {POLARSSL_CIPHER_CAMELLIA_256_ECB,    16},
        {POLARSSL_CIPHER_CAMELLIA_128_CBC,    16},
        {POLARSSL_CIPHER_CAMELLIA_192_CBC,    16},
        {POLARSSL_CIPHER_CAMELLIA_256_CBC,    16},
        {POLARSSL_CIPHER_CAMELLIA_128_CFB128, 16},
        {POLARSSL_CIPHER_CAMELLIA_192_CFB128, 16},
        {POLARSSL_CIPHER_CAMELLIA_256_CFB128, 16},
        {POLARSSL_CIPHER_CAMELLIA_128_CTR,    16},
        {POLARSSL_CIPHER_CAMELLIA_192_CTR,    16},
        {POLARSSL_CIPHER_CAMELLIA_256_CTR,    16},
        {POLARSSL_CIPHER_CAMELLIA_128_GCM,    16},
        {POLARSSL_CIPHER_CAMELLIA_192_GCM,    16},
        {POLARSSL_CIPHER_CAMELLIA_256_GCM,    16},
    #endif

    #ifdef POLARSSL_DES_C
        {POLARSSL_CIPHER_DES_ECB,              8},
        {POLARSSL_CIPHER_DES_CBC,              8},
        {POLARSSL_CIPHER_DES_EDE_ECB,          8},
        {POLARSSL_CIPHER_DES_EDE_CBC,          8},
        {POLARSSL_CIPHER_DES_EDE3_ECB,         8},
        {POLARSSL_CIPHER_DES_EDE3_CBC,         8},
    #endif

    #ifdef POLARSSL_BLOWFISH_C
        {POLARSSL_CIPHER_BLOWFISH_ECB,         8},
        {POLARSSL_CIPHER_BLOWFISH_CBC,         8},
        {POLARSSL_CIPHER_BLOWFISH_CFB64,       8},
        {POLARSSL_CIPHER_BLOWFISH_CTR,         8},
    #endif

    #ifdef POLARSSL_ARC4_C
        {POLARSSL_CIPHER_ARC4_128,             1},
    #endif

    #ifdef POLARSSL_CCM_C
        {POLARSSL_CIPHER_AES_128_CCM,         16},
        {POLARSSL_CIPHER_AES_192_CCM,         16},
        {POLARSSL_CIPHER_AES_256_CCM,         16},
        {POLARSSL_CIPHER_CAMELLIA_128_CCM,    16},
        {POLARSSL_CIPHER_CAMELLIA_192_CCM,    16},
        {POLARSSL_CIPHER_CAMELLIA_256_CCM,    16},
    #endif
    };

    int i;
    for (i = 0; i < _countof(algorithms); i++)
    {
        if (algorithms[i].algo == cipher_suite->cipher)
            return algorithms[i].size;
    }

    TRACE("POLARSSL schannel_get_cipher_block_size: Unknown cipher %#x, returning 1\n", ciphersuite_id);

    return 1;
}

static unsigned int schannel_get_cipher_key_size(int ciphersuite_id)
{
    const ssl_ciphersuite_t *cipher_suite = ssl_ciphersuite_from_id(ciphersuite_id);
    const struct
    {
        int algo;
        unsigned int size;
    }
    algorithms[] =
    {
        {POLARSSL_CIPHER_NONE,                  0},
        {POLARSSL_CIPHER_NULL,                  0},

    #ifdef POLARSSL_AES_C
        {POLARSSL_CIPHER_AES_128_ECB,         128},
        {POLARSSL_CIPHER_AES_192_ECB,         192},
        {POLARSSL_CIPHER_AES_256_ECB,         256},
        {POLARSSL_CIPHER_AES_128_CBC,         128},
        {POLARSSL_CIPHER_AES_192_CBC,         192},
        {POLARSSL_CIPHER_AES_256_CBC,         256},
        {POLARSSL_CIPHER_AES_128_CFB128,      128},
        {POLARSSL_CIPHER_AES_192_CFB128,      192},
        {POLARSSL_CIPHER_AES_256_CFB128,      256},
        {POLARSSL_CIPHER_AES_128_CTR,         128},
        {POLARSSL_CIPHER_AES_192_CTR,         192},
        {POLARSSL_CIPHER_AES_256_CTR,         256},
        {POLARSSL_CIPHER_AES_128_GCM,         128},
        {POLARSSL_CIPHER_AES_192_GCM,         192},
        {POLARSSL_CIPHER_AES_256_GCM,         256},
    #endif

    #ifdef POLARSSL_CAMELLIA_C
        {POLARSSL_CIPHER_CAMELLIA_128_ECB,    128},
        {POLARSSL_CIPHER_CAMELLIA_192_ECB,    192},
        {POLARSSL_CIPHER_CAMELLIA_256_ECB,    256},
        {POLARSSL_CIPHER_CAMELLIA_128_CBC,    128},
        {POLARSSL_CIPHER_CAMELLIA_192_CBC,    192},
        {POLARSSL_CIPHER_CAMELLIA_256_CBC,    256},
        {POLARSSL_CIPHER_CAMELLIA_128_CFB128, 128},
        {POLARSSL_CIPHER_CAMELLIA_192_CFB128, 192},
        {POLARSSL_CIPHER_CAMELLIA_256_CFB128, 256},
        {POLARSSL_CIPHER_CAMELLIA_128_CTR,    128},
        {POLARSSL_CIPHER_CAMELLIA_192_CTR,    192},
        {POLARSSL_CIPHER_CAMELLIA_256_CTR,    256},
        {POLARSSL_CIPHER_CAMELLIA_128_GCM,    128},
        {POLARSSL_CIPHER_CAMELLIA_192_GCM,    192},
        {POLARSSL_CIPHER_CAMELLIA_256_GCM,    256},
    #endif

    #ifdef POLARSSL_DES_C
        {POLARSSL_CIPHER_DES_ECB,              56},
        {POLARSSL_CIPHER_DES_CBC,              56},
        {POLARSSL_CIPHER_DES_EDE_ECB,         128},
        {POLARSSL_CIPHER_DES_EDE_CBC,         128},
        {POLARSSL_CIPHER_DES_EDE3_ECB,        192},
        {POLARSSL_CIPHER_DES_EDE3_CBC,        192},
    #endif

    #ifdef POLARSSL_BLOWFISH_C                      /* lies! actually unlimited,    */
        {POLARSSL_CIPHER_BLOWFISH_ECB,        128}, /* gnutls uses this same value. */
        {POLARSSL_CIPHER_BLOWFISH_CBC,        128}, /* BLOWFISH_MAX_KEY = 448 bits  */
        {POLARSSL_CIPHER_BLOWFISH_CFB64,      128}, /* BLOWFISH_MIN_KEY = 032 bits  */
        {POLARSSL_CIPHER_BLOWFISH_CTR,        128}, /* see blowfish.h for more info */
    #endif

    #ifdef POLARSSL_ARC4_C
        {POLARSSL_CIPHER_ARC4_128,            128},
    #endif

    #ifdef POLARSSL_CCM_C
        {POLARSSL_CIPHER_AES_128_CCM,         128},
        {POLARSSL_CIPHER_AES_192_CCM,         192},
        {POLARSSL_CIPHER_AES_256_CCM,         256},
        {POLARSSL_CIPHER_CAMELLIA_128_CCM,    128},
        {POLARSSL_CIPHER_CAMELLIA_192_CCM,    192},
        {POLARSSL_CIPHER_CAMELLIA_256_CCM,    256},
    #endif
    };

    int i;

    for (i = 0; i < _countof(algorithms); i++)
    {
        if (algorithms[i].algo == cipher_suite->cipher)
            return algorithms[i].size;
    }

    TRACE("POLARSSL schannel_get_cipher_key_size: Unknown cipher %#x, returning 0\n", ciphersuite_id);

    return 0;
}

static unsigned int schannel_get_mac_key_size(int ciphersuite_id)
{
    const ssl_ciphersuite_t *cipher_suite = ssl_ciphersuite_from_id(ciphersuite_id);
    const unsigned int algorithms[] =
    {
          0, /* POLARSSL_MD_NONE      */
          0, /* POLARSSL_MD_MD2 (not used as MAC) */
        128, /* POLARSSL_MD_MD4       */
        128, /* POLARSSL_MD_MD5       */
        160, /* POLARSSL_MD_SHA1      */
        224, /* POLARSSL_MD_SHA224    */
        256, /* POLARSSL_MD_SHA256    */
        384, /* POLARSSL_MD_SHA384    */
        512, /* POLARSSL_MD_SHA512    */
        160  /* POLARSSL_MD_RIPEMD160 */
    };

    if (cipher_suite->mac >= 0 && cipher_suite->mac < _countof(algorithms))
    {
        return algorithms[cipher_suite->mac];
    }

    TRACE("POLARSSL schannel_get_mac_key_size: Unknown mac %#x for ciphersuite %#x, returning 0\n", cipher_suite->mac, ciphersuite_id);

    return 0;
}

static unsigned int schannel_get_kx_key_size(const ssl_context *ssl, int ciphersuite_id)
{
    const ssl_ciphersuite_t *cipher_suite = ssl_ciphersuite_from_id(ciphersuite_id);

    /* if we are the server take ca_chain, if we are the client take server cert (peer_cert) */
    x509_crt *server_cert = (ssl->endpoint == SSL_IS_SERVER) ? ssl->ca_chain : ssl->session->peer_cert;

    if (cipher_suite->key_exchange != POLARSSL_KEY_EXCHANGE_NONE)
        return server_cert->pk.pk_info->get_size(server_cert->pk.pk_ctx);

    TRACE("POLARSSL schannel_get_kx_key_size: Unknown kx %#x, returning 0\n", cipher_suite->key_exchange);

    return 0;
}

static DWORD schannel_get_protocol(const ssl_context *ssl)
{
    /* FIXME: currently schannel only implements client connections, but
     * there's no reason it couldn't be used for servers as well. The
     * context doesn't tell us which it is, so decide based on ssl endpoint value. */

    switch (ssl->minor_ver)
    {
        case SSL_MINOR_VERSION_0:
            return (ssl->endpoint == SSL_IS_CLIENT) ? SP_PROT_SSL3_CLIENT :
                                                      SP_PROT_SSL3_SERVER;

        case SSL_MINOR_VERSION_1:
            return (ssl->endpoint == SSL_IS_CLIENT) ? SP_PROT_TLS1_0_CLIENT :
                                                      SP_PROT_TLS1_0_SERVER;

        case SSL_MINOR_VERSION_2:
            return (ssl->endpoint == SSL_IS_CLIENT) ? SP_PROT_TLS1_1_CLIENT :
                                                      SP_PROT_TLS1_1_SERVER;

        case SSL_MINOR_VERSION_3:
            return (ssl->endpoint == SSL_IS_CLIENT) ? SP_PROT_TLS1_2_CLIENT :
                                                      SP_PROT_TLS1_2_SERVER;

        default:
        {
            FIXME("POLARSSL schannel_get_protocol: unknown protocol %d\n", ssl->minor_ver);
            return 0;
        }
    }
}

static ALG_ID schannel_get_cipher_algid(int ciphersuite_id)
{
    const ssl_ciphersuite_t *cipher_suite = ssl_ciphersuite_from_id(ciphersuite_id);

    switch (cipher_suite->cipher)
    {
        case POLARSSL_CIPHER_NONE:
        case POLARSSL_CIPHER_NULL:
            return 0;

#ifdef POLARSSL_ARC4_C
        /* ARC4 */
        case POLARSSL_CIPHER_ARC4_128:
            return CALG_RC4;
#endif


#ifdef POLARSSL_DES_C
        /* DES */
        case POLARSSL_CIPHER_DES_ECB:
        case POLARSSL_CIPHER_DES_CBC:
        case POLARSSL_CIPHER_DES_EDE_ECB:
        case POLARSSL_CIPHER_DES_EDE_CBC:
            return CALG_DES;

        case POLARSSL_CIPHER_DES_EDE3_ECB:
        case POLARSSL_CIPHER_DES_EDE3_CBC:
            return CALG_3DES;
#endif

#ifdef POLARSSL_AES_C
        /* AES 128 */
        case POLARSSL_CIPHER_AES_128_ECB:
        case POLARSSL_CIPHER_AES_128_CBC:
        case POLARSSL_CIPHER_AES_128_CFB128:
        case POLARSSL_CIPHER_AES_128_CTR:
        case POLARSSL_CIPHER_AES_128_GCM:
    #ifdef POLARSSL_CCM_C
        case POLARSSL_CIPHER_AES_128_CCM:
    #endif
            return CALG_AES_128;

        case POLARSSL_CIPHER_AES_192_ECB:
        case POLARSSL_CIPHER_AES_192_CBC:
        case POLARSSL_CIPHER_AES_192_CFB128:
        case POLARSSL_CIPHER_AES_192_CTR:
        case POLARSSL_CIPHER_AES_192_GCM:
    #ifdef POLARSSL_CCM_C
        case POLARSSL_CIPHER_AES_192_CCM:
    #endif
            return CALG_AES_192;

        case POLARSSL_CIPHER_AES_256_ECB:
        case POLARSSL_CIPHER_AES_256_CBC:
        case POLARSSL_CIPHER_AES_256_CFB128:
        case POLARSSL_CIPHER_AES_256_CTR:
        case POLARSSL_CIPHER_AES_256_GCM:
    #ifdef POLARSSL_CCM_C
        case POLARSSL_CIPHER_AES_256_CCM:
    #endif
            return CALG_AES_256;
#endif

        /* nothing to show? fall through */
        default:
        {
            FIXME("POLARSSL schannel_get_cipher_algid: unknown algorithm %d\n", ciphersuite_id);
            return 0;
        }
    }
}

static ALG_ID schannel_get_mac_algid(int ciphersuite_id)
{
    const ssl_ciphersuite_t *cipher_suite = ssl_ciphersuite_from_id(ciphersuite_id);

    switch (cipher_suite->mac)
    {
        case POLARSSL_MD_NONE:      return 0;
        case POLARSSL_MD_MD2:       return CALG_MD2;
        case POLARSSL_MD_MD4:       return CALG_MD4;
        case POLARSSL_MD_MD5:       return CALG_MD5;
        case POLARSSL_MD_SHA1:      return CALG_SHA1;
        case POLARSSL_MD_SHA224:    return CALG_SHA;
        case POLARSSL_MD_SHA256:    return CALG_SHA_256;
        case POLARSSL_MD_SHA384:    return CALG_SHA_384;
        case POLARSSL_MD_SHA512:    return CALG_SHA_512;
        case POLARSSL_MD_RIPEMD160: return (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_RIPEMD160); /* there's no CALG_RIPEMD or CALG_RIPEMD160 defined in <wincrypt.h> yet */

        default:
        {
            FIXME("POLARSSL schannel_get_mac_algid: unknown algorithm %d\n", cipher_suite->mac);
            return 0;
        }
    }
}

static ALG_ID schannel_get_kx_algid(int ciphersuite_id)
{
    const ssl_ciphersuite_t *cipher_suite = ssl_ciphersuite_from_id(ciphersuite_id);

    switch (cipher_suite->key_exchange)
    {
        case POLARSSL_KEY_EXCHANGE_NONE:
            return 0;

        case POLARSSL_KEY_EXCHANGE_ECDH_RSA:
        case POLARSSL_KEY_EXCHANGE_ECDH_ECDSA:
        case POLARSSL_KEY_EXCHANGE_ECDHE_PSK:
        case POLARSSL_KEY_EXCHANGE_ECDHE_RSA:
            return CALG_ECDH;

        case POLARSSL_KEY_EXCHANGE_RSA_PSK:
        case POLARSSL_KEY_EXCHANGE_RSA:
            return CALG_RSA_KEYX;

        case POLARSSL_KEY_EXCHANGE_DHE_PSK:
        case POLARSSL_KEY_EXCHANGE_DHE_RSA:
            return CALG_DH_EPHEM;

        default:
        {
            FIXME("POLARSSL schannel_get_kx_algid: unknown algorithm %d\n", cipher_suite->key_exchange);
            return 0;
        }
    }
}

unsigned int schan_imp_get_session_cipher_block_size(schan_imp_session session)
{
    POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;
    TRACE("POLARSSL schan_imp_get_session_cipher_block_size %p.\n", session);

    return schannel_get_cipher_block_size(ssl_get_ciphersuite_id(ssl_get_ciphersuite(&s->ssl)));
}

unsigned int schan_imp_get_max_message_size(schan_imp_session session)
{
    // POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;
    TRACE("POLARSSL schan_imp_get_max_message_size %p.\n", session);

    /* FIXME: update to mbedTLS 2.1.0 to use this function */
    // return mbedtls_ssl_get_max_frag_len(&s->ssl);
    return SSL_MAX_CONTENT_LEN;
}

SECURITY_STATUS schan_imp_get_connection_info(schan_imp_session session,
                                              SecPkgContext_ConnectionInfo *info)
{
    POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;

    int ciphersuite_id = ssl_get_ciphersuite_id(ssl_get_ciphersuite(&s->ssl));

    TRACE("POLARSSL schan_imp_get_connection_info %p %p.\n", session, info);

    info->dwProtocol       = schannel_get_protocol(&s->ssl);
    info->aiCipher         = schannel_get_cipher_algid(ciphersuite_id);
    info->dwCipherStrength = schannel_get_cipher_key_size(ciphersuite_id);
    info->aiHash           = schannel_get_mac_algid(ciphersuite_id);
    info->dwHashStrength   = schannel_get_mac_key_size(ciphersuite_id);
    info->aiExch           = schannel_get_kx_algid(ciphersuite_id);
    info->dwExchStrength   = schannel_get_kx_key_size(&s->ssl, ciphersuite_id);

    return SEC_E_OK;
}

SECURITY_STATUS schan_imp_get_session_peer_certificate(schan_imp_session session, HCERTSTORE store,
                                                       PCCERT_CONTEXT *ret)
{
    POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;
    PCCERT_CONTEXT cert_context = NULL;
    const x509_crt *next_cert;

    TRACE("POLARSSL schan_imp_get_session_peer_certificate %p %p %p %p.\n", session, store, ret, ret != NULL ? *ret : NULL);

    if (!s->ssl.session->peer_cert)
        return SEC_E_INTERNAL_ERROR;

    for (next_cert = s->ssl.session->peer_cert; next_cert != NULL; next_cert = next_cert->next)
    {
        if (!CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING, next_cert->raw.p, next_cert->raw.len,
            CERT_STORE_ADD_REPLACE_EXISTING, (next_cert != s->ssl.session->peer_cert) ? NULL : &cert_context))
        {
            if (next_cert != s->ssl.session->peer_cert)
                CertFreeCertificateContext(cert_context);
            return GetLastError();
        }
    }

    *ret = cert_context;
    return SEC_E_OK;
}

SECURITY_STATUS schan_imp_send(schan_imp_session session, const void *buffer,
                               SIZE_T *length)
{
    POLARSSL_SESSION *s = (POLARSSL_SESSION *)session;
    int ret;

    ret = ssl_write(&s->ssl, (unsigned char *)buffer, *length);

    TRACE("POLARSSL schan_imp_send: (%p, %p, %p/%lu)\n", s, buffer, length, *length);

    if (ret >= 0)
    {
        TRACE("POLARSSL schan_imp_send: ret=%i.\n", ret);

        *length = ret;
    }
    else if ((ret & ROS_SCHAN_IS_BLOCKING) == ROS_SCHAN_IS_BLOCKING)
    {
        *length = ROS_SCHAN_IS_BLOCKING_RETRIEVE(ret);

        if (!*length)
        {
            TRACE("POLARSSL schan_imp_send: ret=POLARSSL_ERR_NET_WANT_WRITE -> SEC_I_CONTINUE_NEEDED; len=%lu", *length);
            return SEC_I_CONTINUE_NEEDED;
        }
        else
        {
            TRACE("POLARSSL schan_imp_send: ret=POLARSSL_ERR_NET_WANT_WRITE -> SEC_E_OK; len=%lu", *length);
            return SEC_E_OK;
        }
    }
    else
    {
        ERR("POLARSSL schan_imp_send: ssl_write failed with -%x\n", -ret);
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

SECURITY_STATUS schan_imp_recv(schan_imp_session session, void *buffer,
                               SIZE_T *length)
{
    PPOLARSSL_SESSION s = (PPOLARSSL_SESSION)session;
    int ret;

    TRACE("POLARSSL schan_imp_recv: (%p, %p, %p/%lu)\n", s, buffer, length, *length);

    ret = ssl_read(&s->ssl, (unsigned char *)buffer, *length);

    TRACE("POLARSSL schan_imp_recv: (%p, %p, %p/%lu) ret= %#x\n", s, buffer, length, *length, ret);

    if (ret >= 0)
    {
        TRACE("POLARSSL schan_imp_recv: ret == %i.\n", ret);

        *length = ret;
    }
    else if ((ret & ROS_SCHAN_IS_BLOCKING) == ROS_SCHAN_IS_BLOCKING)
    {
        *length = ROS_SCHAN_IS_BLOCKING_RETRIEVE(ret);

        if (!*length)
        {
            TRACE("POLARSSL schan_imp_recv: ret=POLARSSL_ERR_NET_WANT_WRITE -> SEC_I_CONTINUE_NEEDED; len=%lu", *length);
            return SEC_I_CONTINUE_NEEDED;
        }
        else
        {
            TRACE("POLARSSL schan_imp_recv: ret=POLARSSL_ERR_NET_WANT_WRITE -> SEC_E_OK; len=%lu", *length);
            return SEC_E_OK;
        }
    }
    else if (ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY)
    {
        TRACE("POLARSSL schan_imp_recv: ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY -> SEC_E_OK\n");
        return SEC_E_OK;
    }
    else
    {
        ERR("POLARSSL schan_imp_recv: ssl_read failed with -%x\n", -ret);
        return SEC_E_INTERNAL_ERROR;
    }

    return SEC_E_OK;
}

BOOL schan_imp_allocate_certificate_credentials(schan_credentials *c)
{
    TRACE("POLARSSL schan_imp_allocate_certificate_credentials %p %p %d\n", c, c->credentials, c->credential_use);

    /* in our case credentials aren't really used for anything, so just stub them */
    c->credentials = NULL;
    return TRUE;
}

void schan_imp_free_certificate_credentials(schan_credentials *c)
{
    TRACE("POLARSSL schan_imp_free_certificate_credentials %p %p %d\n", c, c->credentials, c->credential_use);
}

BOOL schan_imp_init(void)
{
    TRACE("Schannel POLARSSL schan_imp_init\n");
    return TRUE;
}

void schan_imp_deinit(void)
{
    WARN("Schannel POLARSSL schan_imp_deinit\n");
}

#endif /* SONAME_LIBMBEDTLS && !HAVE_SECURITY_SECURITY_H && !SONAME_LIBGNUTLS */