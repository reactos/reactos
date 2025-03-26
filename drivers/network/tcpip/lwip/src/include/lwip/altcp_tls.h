/**
 * @file
 * Application layered TCP/TLS connection API (to be used from TCPIP thread)
 *
 * @defgroup altcp_tls TLS layer
 * @ingroup altcp
 * This file contains function prototypes for a TLS layer.
 * A port to ARM mbedtls is provided in the apps/ tree
 * (LWIP_ALTCP_TLS_MBEDTLS option).
 */

/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */
#ifndef LWIP_HDR_ALTCP_TLS_H
#define LWIP_HDR_ALTCP_TLS_H

#include "lwip/opt.h"

#if LWIP_ALTCP /* don't build if not configured for use in lwipopts.h */

#if LWIP_ALTCP_TLS

#include "lwip/altcp.h"

/* check if mbedtls port is enabled */
#include "lwip/apps/altcp_tls_mbedtls_opts.h"
/* allow session structure to be fully defined when using mbedtls port */
#if LWIP_ALTCP_TLS_MBEDTLS
#include "mbedtls/ssl.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @ingroup altcp_tls
 * ALTCP_TLS configuration handle, content depends on port (e.g. mbedtls)
 */
struct altcp_tls_config;

/** @ingroup altcp_tls
 * Create an ALTCP_TLS server configuration handle prepared for multiple certificates
 */
struct altcp_tls_config *altcp_tls_create_config_server(u8_t cert_count);

/** @ingroup altcp_tls
 * Add a certificate to an ALTCP_TLS server configuration handle
 */
err_t altcp_tls_config_server_add_privkey_cert(struct altcp_tls_config *config,
      const u8_t *privkey, size_t privkey_len,
      const u8_t *privkey_pass, size_t privkey_pass_len,
      const u8_t *cert, size_t cert_len);

/** @ingroup altcp_tls
 * Create an ALTCP_TLS server configuration handle with one certificate
 * (short version of calling @ref altcp_tls_create_config_server and
 * @ref altcp_tls_config_server_add_privkey_cert)
 */
struct altcp_tls_config *altcp_tls_create_config_server_privkey_cert(const u8_t *privkey, size_t privkey_len,
                            const u8_t *privkey_pass, size_t privkey_pass_len,
                            const u8_t *cert, size_t cert_len);

/** @ingroup altcp_tls
 * Create an ALTCP_TLS client configuration handle
 */
struct altcp_tls_config *altcp_tls_create_config_client(const u8_t *cert, size_t cert_len);

/** @ingroup altcp_tls
 * Create an ALTCP_TLS client configuration handle with two-way server/client authentication
 */
struct altcp_tls_config *altcp_tls_create_config_client_2wayauth(const u8_t *ca, size_t ca_len, const u8_t *privkey, size_t privkey_len,
                            const u8_t *privkey_pass, size_t privkey_pass_len,
                            const u8_t *cert, size_t cert_len);

/** @ingroup altcp_tls
 * Configure ALPN TLS extension
 * Example:<br>
 * static const char *g_alpn_protocols[] = { "x-amzn-mqtt-ca", NULL };<br>
 * tls_config = altcp_tls_create_config_client(ca, ca_len);<br>
 * altcp_tls_conf_alpn_protocols(tls_config, g_alpn_protocols);<br>
 */
int altcp_tls_configure_alpn_protocols(struct altcp_tls_config *conf, const char **protos);

/** @ingroup altcp_tls
 * Free an ALTCP_TLS configuration handle
 */
void altcp_tls_free_config(struct altcp_tls_config *conf);

/** @ingroup altcp_tls
 * Free an ALTCP_TLS global entropy instance.
 * All ALTCP_TLS configuration are linked to one altcp_tls_entropy_rng structure
 * that handle an unique system entropy & ctr_drbg instance.
 * This function allow application to free this altcp_tls_entropy_rng structure
 * when all configuration referencing it were destroyed.
 * This function does nothing if some ALTCP_TLS configuration handle are still
 * active.
 */
void altcp_tls_free_entropy(void);

/** @ingroup altcp_tls
 * Create new ALTCP_TLS layer wrapping an existing pcb as inner connection (e.g. TLS over TCP)
 */
struct altcp_pcb *altcp_tls_wrap(struct altcp_tls_config *config, struct altcp_pcb *inner_pcb);

/** @ingroup altcp_tls
 * Create new ALTCP_TLS pcb and its inner tcp pcb
 */
struct altcp_pcb *altcp_tls_new(struct altcp_tls_config *config, u8_t ip_type);

/** @ingroup altcp_tls
 * Create new ALTCP_TLS layer pcb and its inner tcp pcb.
 * Same as @ref altcp_tls_new but this allocator function fits to
 * @ref altcp_allocator_t / @ref altcp_new.<br>
 'arg' must contain a struct altcp_tls_config *.
 */
struct altcp_pcb *altcp_tls_alloc(void *arg, u8_t ip_type);

/** @ingroup altcp_tls
 * Return pointer to internal TLS context so application can tweak it.
 * Real type depends on port (e.g. mbedtls)
 */
void *altcp_tls_context(struct altcp_pcb *conn);

/** @ingroup altcp_tls
 * ALTCP_TLS session handle, content depends on port (e.g. mbedtls)
 */
struct altcp_tls_session
#if LWIP_ALTCP_TLS_MBEDTLS
{
    mbedtls_ssl_session data;
}
#endif
;

/** @ingroup altcp_tls
 * Initialise a TLS session buffer.
 * Real type depends on port (e.g. mbedtls use mbedtls_ssl_session)
 */
void altcp_tls_init_session(struct altcp_tls_session *dest);

/** @ingroup altcp_tls
 * Save current connected session to reuse it later. Should be called after altcp_connect() succeeded.
 * Return error if saving session fail.
 * Real type depends on port (e.g. mbedtls use mbedtls_ssl_session)
 */
err_t altcp_tls_get_session(struct altcp_pcb *conn, struct altcp_tls_session *dest);

/** @ingroup altcp_tls
 * Restore a previously saved session. Must be called before altcp_connect().
 * Return error if cannot restore session.
 * Real type depends on port (e.g. mbedtls use mbedtls_ssl_session)
 */
err_t altcp_tls_set_session(struct altcp_pcb *conn, struct altcp_tls_session *from);

/** @ingroup altcp_tls
 * Free allocated data inside a TLS session buffer.
 * Real type depends on port (e.g. mbedtls use mbedtls_ssl_session)
 */
void altcp_tls_free_session(struct altcp_tls_session *dest);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_ALTCP_TLS */
#endif /* LWIP_ALTCP */
#endif /* LWIP_HDR_ALTCP_TLS_H */
