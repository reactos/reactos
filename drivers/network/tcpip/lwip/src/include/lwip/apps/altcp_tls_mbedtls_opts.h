/**
 * @file
 * Application layered TCP/TLS connection API (to be used from TCPIP thread)
 *
 * This file contains options for an mbedtls port of the TLS layer.
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
#ifndef LWIP_HDR_ALTCP_TLS_OPTS_H
#define LWIP_HDR_ALTCP_TLS_OPTS_H

#include "lwip/opt.h"

#if LWIP_ALTCP /* don't build if not configured for use in lwipopts.h */

/** LWIP_ALTCP_TLS_MBEDTLS==1: use mbedTLS for TLS support for altcp API
 * mbedtls include directory must be reachable via include search path
 */
#ifndef LWIP_ALTCP_TLS_MBEDTLS
#define LWIP_ALTCP_TLS_MBEDTLS                        0
#endif

/** Configure debug level of this file */
#ifndef ALTCP_MBEDTLS_DEBUG
#define ALTCP_MBEDTLS_DEBUG                           LWIP_DBG_OFF
#endif

/** Configure lwIP debug level of the mbedTLS library */
#ifndef ALTCP_MBEDTLS_LIB_DEBUG
#define ALTCP_MBEDTLS_LIB_DEBUG                       LWIP_DBG_OFF
#endif

/** Configure minimum internal debug level of the mbedTLS library */
#ifndef ALTCP_MBEDTLS_LIB_DEBUG_LEVEL_MIN
#define ALTCP_MBEDTLS_LIB_DEBUG_LEVEL_MIN             0
#endif

/** Enable the basic session cache
 * ATTENTION: Using a session cache can lower security by reusing keys!
 */
#ifndef ALTCP_MBEDTLS_USE_SESSION_CACHE
#define ALTCP_MBEDTLS_USE_SESSION_CACHE               0
#endif

/** Maximum cache size of the basic session cache */
#ifndef ALTCP_MBEDTLS_SESSION_CACHE_SIZE
#define ALTCP_MBEDTLS_SESSION_CACHE_SIZE              30
#endif

/** Set a session timeout in seconds for the basic session cache  */
#ifndef ALTCP_MBEDTLS_SESSION_CACHE_TIMEOUT_SECONDS
#define ALTCP_MBEDTLS_SESSION_CACHE_TIMEOUT_SECONDS   (60 * 60)
#endif

/** Use session tickets to speed up connection setup (needs
 * MBEDTLS_SSL_SESSION_TICKETS enabled in mbedTLS config).
 * ATTENTION: Using session tickets can lower security by reusing keys!
 */
#ifndef ALTCP_MBEDTLS_USE_SESSION_TICKETS
#define ALTCP_MBEDTLS_USE_SESSION_TICKETS             0
#endif

/** Session ticket cipher */
#ifndef ALTCP_MBEDTLS_SESSION_TICKET_CIPHER
#define ALTCP_MBEDTLS_SESSION_TICKET_CIPHER           MBEDTLS_CIPHER_AES_256_GCM
#endif

/** Maximum timeout for session tickets */
#ifndef ALTCP_MBEDTLS_SESSION_TICKET_TIMEOUT_SECONDS
#define ALTCP_MBEDTLS_SESSION_TICKET_TIMEOUT_SECONDS  (60 * 60 * 24)
#endif

/** Certificate verification mode: MBEDTLS_SSL_VERIFY_NONE, MBEDTLS_SSL_VERIFY_OPTIONAL (default),
 * MBEDTLS_SSL_VERIFY_REQUIRED (recommended)*/
#ifndef ALTCP_MBEDTLS_AUTHMODE
#define ALTCP_MBEDTLS_AUTHMODE                        MBEDTLS_SSL_VERIFY_OPTIONAL
#endif

#endif /* LWIP_ALTCP */

#endif /* LWIP_HDR_ALTCP_TLS_OPTS_H */
