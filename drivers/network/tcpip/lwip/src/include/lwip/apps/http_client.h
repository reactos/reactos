/**
 * @file
 * HTTP client
 */

/*
 * Copyright (c) 2018 Simon Goldschmidt <goldsimon@gmx.de>
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

#ifndef LWIP_HDR_APPS_HTTP_CLIENT_H
#define LWIP_HDR_APPS_HTTP_CLIENT_H

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/altcp.h"
#include "lwip/prot/iana.h"
#include "lwip/pbuf.h"

#if LWIP_TCP && LWIP_CALLBACK_API

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup httpc
 * HTTPC_HAVE_FILE_IO: define this to 1 to have functions dowloading directly
 * to disk via fopen/fwrite.
 * These functions are example implementations of the interface only.
 */
#ifndef LWIP_HTTPC_HAVE_FILE_IO
#define LWIP_HTTPC_HAVE_FILE_IO   0
#endif

/**
 * @ingroup httpc
 * The default TCP port used for HTTP
 */
#define HTTP_DEFAULT_PORT         LWIP_IANA_PORT_HTTP

/**
 * @ingroup httpc
 * HTTP client result codes
 */
typedef enum ehttpc_result {
  /** File successfully received */
  HTTPC_RESULT_OK            = 0,
  /** Unknown error */
  HTTPC_RESULT_ERR_UNKNOWN   = 1,
  /** Connection to server failed */
  HTTPC_RESULT_ERR_CONNECT   = 2,
  /** Failed to resolve server hostname */
  HTTPC_RESULT_ERR_HOSTNAME  = 3,
  /** Connection unexpectedly closed by remote server */
  HTTPC_RESULT_ERR_CLOSED    = 4,
  /** Connection timed out (server didn't respond in time) */
  HTTPC_RESULT_ERR_TIMEOUT   = 5,
  /** Server responded with an error code */
  HTTPC_RESULT_ERR_SVR_RESP  = 6,
  /** Local memory error */
  HTTPC_RESULT_ERR_MEM       = 7,
  /** Local abort */
  HTTPC_RESULT_LOCAL_ABORT   = 8,
  /** Content length mismatch */
  HTTPC_RESULT_ERR_CONTENT_LEN = 9
} httpc_result_t;

typedef struct _httpc_state httpc_state_t;

/**
 * @ingroup httpc
 * Prototype of a http client callback function
 *
 * @param arg argument specified when initiating the request
 * @param httpc_result result of the http transfer (see enum httpc_result_t)
 * @param rx_content_len number of bytes received (without headers)
 * @param srv_res this contains the http status code received (if any)
 * @param err an error returned by internal lwip functions, can help to specify
 *            the source of the error but must not necessarily be != ERR_OK
 */
typedef void (*httpc_result_fn)(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);

/**
 * @ingroup httpc
 * Prototype of http client callback: called when the headers are received
 *
 * @param connection http client connection
 * @param arg argument specified when initiating the request
 * @param hdr header pbuf(s) (may contain data also)
 * @param hdr_len length of the heders in 'hdr'
 * @param content_len content length as received in the headers (-1 if not received)
 * @return if != ERR_OK is returned, the connection is aborted
 */
typedef err_t (*httpc_headers_done_fn)(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);

typedef struct _httpc_connection {
  ip_addr_t proxy_addr;
  u16_t proxy_port;
  u8_t use_proxy;
  /* @todo: add username:pass? */

#if LWIP_ALTCP
  altcp_allocator_t *altcp_allocator;
#endif

  /* this callback is called when the transfer is finished (or aborted) */
  httpc_result_fn result_fn;
  /* this callback is called after receiving the http headers
     It can abort the connection by returning != ERR_OK */
  httpc_headers_done_fn headers_done_fn;
} httpc_connection_t;

err_t httpc_get_file(const ip_addr_t* server_addr, u16_t port, const char* uri, const httpc_connection_t *settings,
                     altcp_recv_fn recv_fn, void* callback_arg, httpc_state_t **connection);
err_t httpc_get_file_dns(const char* server_name, u16_t port, const char* uri, const httpc_connection_t *settings,
                     altcp_recv_fn recv_fn, void* callback_arg, httpc_state_t **connection);

#if LWIP_HTTPC_HAVE_FILE_IO
err_t httpc_get_file_to_disk(const ip_addr_t* server_addr, u16_t port, const char* uri, const httpc_connection_t *settings,
                     void* callback_arg, const char* local_file_name, httpc_state_t **connection);
err_t httpc_get_file_dns_to_disk(const char* server_name, u16_t port, const char* uri, const httpc_connection_t *settings,
                     void* callback_arg, const char* local_file_name, httpc_state_t **connection);
#endif /* LWIP_HTTPC_HAVE_FILE_IO */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_TCP && LWIP_CALLBACK_API */

#endif /* LWIP_HDR_APPS_HTTP_CLIENT_H */
